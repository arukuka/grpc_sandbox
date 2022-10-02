load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

proto_library(
    name = "data_transfer_proto",
    srcs = ["data_transfer.proto"],
    deps = ["@com_google_protobuf//:empty_proto"],
)

cc_proto_library(
    name = "data_transfer_cc_proto",
    deps = [":data_transfer_proto"],
)

cc_grpc_library(
    name = "data_transfer_cc_grpc",
    srcs = [":data_transfer_proto"],
    grpc_only = True,
    deps = [
        ":data_transfer_cc_proto",
        "@com_github_grpc_grpc//:grpc++",
        "@boost//:multiprecision",
        "@boost//:serialization",
    ],
    visibility = ["//visibility:public"],
)

cmake(
    name = "indicators",
    lib_source = "@indicators//:all_srcs",
    out_headers_only = True,
)

cc_binary(
    name = "server",
    srcs = ["server.cpp"],
    deps = [
        ":data_transfer_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@boost//:multiprecision",
        "@boost//:serialization",
        "@boost//:format",
    ],
)

cc_binary(
    name = "client",
    srcs = ["client.cpp"],
    deps = [
        ":data_transfer_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@boost//:format",
        "@boost//:program_options",
        ":indicators",
    ],
)

cc_binary(
    name = "generate_test_casse",
    srcs = ["generate_test_casse.cpp"],
    deps = [
        "@boost//:serialization",
        "@boost//:multiprecision",
        "@boost//:program_options",
    ],
)

load("@com_grail_bazel_compdb//:defs.bzl", "compilation_database")
load("@com_grail_bazel_output_base_util//:defs.bzl", "OUTPUT_BASE")

compilation_database(
    name = "compdb",
    targets = [
        "//:server",
        "//:client",
        "//:generate_test_casse",
    ],
    output_base = OUTPUT_BASE,
)
