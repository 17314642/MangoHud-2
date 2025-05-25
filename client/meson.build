src = [
    'main.cpp',
    '../common/socket.cpp'
]

link_args = cpp.get_supported_link_arguments([
	'-Wl,-Bsymbolic-functions',
	'-Wl,-z,relro',
	'-Wl,--exclude-libs,ALL',
	'-static-libstdc++'
])

#executable('mangohud-client', src, dependencies: spdlog_dep)
mangohud_client_static_lib = static_library(
	'mangohud-client',
	src,
	dependencies: spdlog_dep,
	gnu_symbol_visibility: 'hidden',
	link_args: link_args
)

shared_library(
	'mangohud-client',
	objects: mangohud_client_static_lib.extract_all_objects(recursive: true),
    link_with: mangohud_client_static_lib,
	link_args: link_args
)
