// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/d3d12/PipelineLayoutD3D12.h"

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"

using Microsoft::WRL::ComPtr;

namespace dawn_native { namespace d3d12 {
    namespace {
        D3D12_SHADER_VISIBILITY ShaderVisibilityType(wgpu::ShaderStage visibility) {
            ASSERT(visibility != wgpu::ShaderStage::None);

            if (visibility == wgpu::ShaderStage::Vertex) {
                return D3D12_SHADER_VISIBILITY_VERTEX;
            }

            if (visibility == wgpu::ShaderStage::Fragment) {
                return D3D12_SHADER_VISIBILITY_PIXEL;
            }

            // For compute or any two combination of stages, visibility must be ALL
            return D3D12_SHADER_VISIBILITY_ALL;
        }

        D3D12_ROOT_PARAMETER_TYPE RootParameterType(wgpu::BindingType type) {
            switch (type) {
                case wgpu::BindingType::UniformBuffer:
                    return D3D12_ROOT_PARAMETER_TYPE_CBV;
                case wgpu::BindingType::StorageBuffer:
                    return D3D12_ROOT_PARAMETER_TYPE_UAV;
                case wgpu::BindingType::ReadonlyStorageBuffer:
                    return D3D12_ROOT_PARAMETER_TYPE_SRV;
                case wgpu::BindingType::SampledTexture:
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                case wgpu::BindingType::StorageTexture:
                case wgpu::BindingType::ReadonlyStorageTexture:
                case wgpu::BindingType::WriteonlyStorageTexture:
                    UNREACHABLE();
            }
        }
    }  // anonymous namespace

    ResultOrError<PipelineLayout*> PipelineLayout::Create(
        Device* device,
        const PipelineLayoutDescriptor* descriptor) {
        Ref<PipelineLayout> layout = AcquireRef(new PipelineLayout(device, descriptor));
        DAWN_TRY(layout->Initialize());
        return layout.Detach();
    }

    MaybeError PipelineLayout::Initialize() {
        Device* device = ToBackend(GetDevice());
        D3D12_ROOT_PARAMETER rootParameters[kMaxBindGroups * 2 + kMaxDynamicBufferCount];

        // A root parameter is one of these types
        union {
            D3D12_ROOT_DESCRIPTOR_TABLE DescriptorTable;
            D3D12_ROOT_CONSTANTS Constants;
            D3D12_ROOT_DESCRIPTOR Descriptor;
        } rootParameterValues[kMaxBindGroups * 2];
        // samplers must be in a separate descriptor table so we need at most twice as many tables
        // as bind groups

        // Ranges are D3D12_DESCRIPTOR_RANGE_TYPE_(SRV|UAV|CBV|SAMPLER)
        // They are grouped together so each bind group has at most 4 ranges
        D3D12_DESCRIPTOR_RANGE ranges[kMaxBindGroups * 4];

        uint32_t parameterIndex = 0;
        uint32_t rangeIndex = 0;

        for (uint32_t group : IterateBitSet(GetBindGroupLayoutsMask())) {
            const BindGroupLayout* bindGroupLayout = ToBackend(GetBindGroupLayout(group));

            // Set the root descriptor table parameter and copy ranges. Ranges are offset by the
            // bind group index Returns whether or not the parameter was set. A root parameter is
            // not set if the number of ranges is 0
            auto SetRootDescriptorTable =
                [&](uint32_t rangeCount, const D3D12_DESCRIPTOR_RANGE* descriptorRanges) -> bool {
                if (rangeCount == 0) {
                    return false;
                }

                auto& rootParameter = rootParameters[parameterIndex];
                rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                rootParameter.DescriptorTable = rootParameterValues[parameterIndex].DescriptorTable;
                rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                rootParameter.DescriptorTable.NumDescriptorRanges = rangeCount;
                rootParameter.DescriptorTable.pDescriptorRanges = &ranges[rangeIndex];

                for (uint32_t i = 0; i < rangeCount; ++i) {
                    ranges[rangeIndex] = descriptorRanges[i];
                    ranges[rangeIndex].RegisterSpace = group;
                    rangeIndex++;
                }

                return true;
            };

            if (SetRootDescriptorTable(bindGroupLayout->GetCbvUavSrvDescriptorTableSize(),
                                       bindGroupLayout->GetCbvUavSrvDescriptorRanges())) {
                mCbvUavSrvRootParameterInfo[group] = parameterIndex++;
            }

            if (SetRootDescriptorTable(bindGroupLayout->GetSamplerDescriptorTableSize(),
                                       bindGroupLayout->GetSamplerDescriptorRanges())) {
                mSamplerRootParameterInfo[group] = parameterIndex++;
            }

            // Get calculated shader register for root descriptors
            const auto& shaderRegisters = bindGroupLayout->GetBindingOffsets();

            // Init root descriptors in root signatures for dynamic buffer bindings.
            // These are packed at the beginning of the layout binding info.
            for (BindingIndex dynamicBindingIndex = 0;
                 dynamicBindingIndex < bindGroupLayout->GetDynamicBufferCount();
                 ++dynamicBindingIndex) {
                const BindingInfo& bindingInfo =
                    bindGroupLayout->GetBindingInfo(dynamicBindingIndex);

                D3D12_ROOT_PARAMETER* rootParameter = &rootParameters[parameterIndex];

                // Setup root descriptor.
                D3D12_ROOT_DESCRIPTOR rootDescriptor;
                rootDescriptor.ShaderRegister = shaderRegisters[dynamicBindingIndex];
                rootDescriptor.RegisterSpace = group;

                // Set root descriptors in root signatures.
                rootParameter->Descriptor = rootDescriptor;
                mDynamicRootParameterIndices[group][dynamicBindingIndex] = parameterIndex++;

                // Set parameter types according to bind group layout descriptor.
                rootParameter->ParameterType = RootParameterType(bindingInfo.type);

                // Set visibilities according to bind group layout descriptor.
                rootParameter->ShaderVisibility = ShaderVisibilityType(bindingInfo.visibility);
            }
        }

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDescriptor;
        rootSignatureDescriptor.NumParameters = parameterIndex;
        rootSignatureDescriptor.pParameters = rootParameters;
        rootSignatureDescriptor.NumStaticSamplers = 0;
        rootSignatureDescriptor.pStaticSamplers = nullptr;
        rootSignatureDescriptor.Flags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        DAWN_TRY(CheckHRESULT(
            device->GetFunctions()->d3d12SerializeRootSignature(
                &rootSignatureDescriptor, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error),
            "D3D12 serialize root signature"));
        DAWN_TRY(CheckHRESULT(device->GetD3D12Device()->CreateRootSignature(
                                  0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                  IID_PPV_ARGS(&mRootSignature)),
                              "D3D12 create root signature"));
        return {};
    }

    uint32_t PipelineLayout::GetCbvUavSrvRootParameterIndex(uint32_t group) const {
        ASSERT(group < kMaxBindGroups);
        return mCbvUavSrvRootParameterInfo[group];
    }

    uint32_t PipelineLayout::GetSamplerRootParameterIndex(uint32_t group) const {
        ASSERT(group < kMaxBindGroups);
        return mSamplerRootParameterInfo[group];
    }

    ID3D12RootSignature* PipelineLayout::GetRootSignature() const {
        return mRootSignature.Get();
    }

    uint32_t PipelineLayout::GetDynamicRootParameterIndex(uint32_t group,
                                                          BindingIndex bindingIndex) const {
        ASSERT(group < kMaxBindGroups);
        ASSERT(bindingIndex < kMaxBindingsPerGroup);
        ASSERT(GetBindGroupLayout(group)->GetBindingInfo(bindingIndex).hasDynamicOffset);
        return mDynamicRootParameterIndices[group][bindingIndex];
    }
}}  // namespace dawn_native::d3d12
