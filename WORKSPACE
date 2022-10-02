load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


http_archive(
    name = "rules_proto",
    sha256 = "c22cfcb3f22a0ae2e684801ea8dfed070ba5bed25e73f73580564f250475e72d",
    strip_prefix = "rules_proto-4.0.0-3.19.2",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0-3.19.2.tar.gz",
    ],
)

http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "e18b16f7976aab9a36c14c38180f042bb0fd196b75c9fd6a20a2b5f934876ad6",
    urls = [
        "https://github.com/grpc/grpc/archive/refs/tags/v1.45.2.tar.gz",
    ],
    strip_prefix = "grpc-1.45.2",
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")


rules_proto_dependencies()
rules_proto_toolchains()
grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

_RULES_BOOST_COMMIT = "630cf5dbad418ee8cfa637b1e33125b11807721d"

http_archive(
    name = "com_github_nelhage_rules_boost",
    sha256 = "8be3ad3578231579f3d289c0330fd90975fe437451830a569a9a35661104a211",
    strip_prefix = "rules_boost-%s" % _RULES_BOOST_COMMIT,
    urls = [
        "https://github.com/nelhage/rules_boost/archive/%s.tar.gz" % _RULES_BOOST_COMMIT,
    ],
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

http_archive(
    name = "rules_foreign_cc",
    sha256 = "2a4d07cd64b0719b39a7c12218a3e507672b82a97b98c6a89d38565894cf7c51",
    strip_prefix = "rules_foreign_cc-0.9.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/refs/tags/0.9.0.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

_ALL_CONTENT = """\
filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
"""

http_archive(
    name = "indicators",
    strip_prefix = "indicators-2.2",
    build_file_content = _ALL_CONTENT,
    urls = [
        "https://github.com/p-ranav/indicators/archive/refs/tags/v2.2.tar.gz",
    ],
    sha256 = "b768f1b7ca64a413503f72d5460cc617c1458c17fb7a8c0ee503d753e1f20d03",
)

http_archive(
    name = "com_grail_bazel_compdb",
    sha256 = "d32835b26dd35aad8fd0ba0d712265df6565a3ad860d39e4c01ad41059ea7eda",
    urls = ["https://github.com/grailbio/bazel-compilation-database/archive/0.5.2.tar.gz"],
    strip_prefix = "bazel-compilation-database-0.5.2",
)

load("@com_grail_bazel_compdb//:deps.bzl", "bazel_compdb_deps")
bazel_compdb_deps()
