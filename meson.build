project('tarea3','cpp',
        default_options : ['cpp_std=c++20'],  
        version : '0.0.2')

# Find Jack dependencies
jack_dep = dependency('jack')
sndfile_dep = dependency('sndfile')
boost_dep = dependency('boost', modules : ['program_options','system'])

all_deps = [jack_dep,sndfile_dep,boost_dep]
sources = files('main.cpp', 'jack_client.cpp','passthrough_client.cpp',
                'sndfile_thread.cpp','waitkey.cpp')

executable('tarea3',sources,dependencies:all_deps)
