project('MangoHud', 'cpp')

spdlog_dep = dependency('spdlog', required : true)

add_project_arguments('-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE', language: 'cpp')

subdir('server')
subdir('client')
