# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: launcher.proto

import sys
_b = sys.version_info[0] < 3 and (lambda x: x) or (lambda x: x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()

DESCRIPTOR = _descriptor.FileDescriptor(
    name='launcher.proto',
    package='buildbucket.v2',
    syntax='proto3',
    serialized_pb=_b(
        '\n\x0elauncher.proto\x12\x0e\x62uildbucket.v2\"#\n\x0c\x42uildSecrets\x12\x13\n\x0b\x62uild_token\x18\x01 \x01(\tB6Z4go.chromium.org/luci/buildbucket/proto;buildbucketpbb\x06proto3'
    )
)
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

_BUILDSECRETS = _descriptor.Descriptor(
    name='BuildSecrets',
    full_name='buildbucket.v2.BuildSecrets',
    filename=None,
    file=DESCRIPTOR,
    containing_type=None,
    fields=[
        _descriptor.FieldDescriptor(
            name='build_token',
            full_name='buildbucket.v2.BuildSecrets.build_token',
            index=0,
            number=1,
            type=9,
            cpp_type=9,
            label=1,
            has_default_value=False,
            default_value=_b("").decode('utf-8'),
            message_type=None,
            enum_type=None,
            containing_type=None,
            is_extension=False,
            extension_scope=None,
            options=None
        ),
    ],
    extensions=[],
    nested_types=[],
    enum_types=[],
    options=None,
    is_extendable=False,
    syntax='proto3',
    extension_ranges=[],
    oneofs=[],
    serialized_start=34,
    serialized_end=69,
)

DESCRIPTOR.message_types_by_name['BuildSecrets'] = _BUILDSECRETS

BuildSecrets = _reflection.GeneratedProtocolMessageType(
    'BuildSecrets',
    (_message.Message,),
    dict(
        DESCRIPTOR=_BUILDSECRETS,
        __module__='launcher_pb2'
        # @@protoc_insertion_point(class_scope:buildbucket.v2.BuildSecrets)
    )
)
_sym_db.RegisterMessage(BuildSecrets)

DESCRIPTOR.has_options = True
DESCRIPTOR._options = _descriptor._ParseOptions(
    descriptor_pb2.FileOptions(),
    _b('Z4go.chromium.org/luci/buildbucket/proto;buildbucketpb')
)
# @@protoc_insertion_point(module_scope)
