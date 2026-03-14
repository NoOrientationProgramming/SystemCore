
project(
	'SystemCore - Introduction',
	'cpp',
)

fs = import('fs')
cxx = meson.get_compiler('cpp')

# Sources

srcs = [
	'../../Processing.cpp',
	'../../Log.cpp',
	'../../SystemDebugging.cpp',
	'../../SystemCommanding.cpp',
	'../../TcpListening.cpp',
	'../../TcpTransfering.cpp',
	'main.cpp',
	'Introducing.cpp',
	'ChildExecuting.cpp',
]

# Arguments

args = [
	'-DCONFIG_PROC_HAVE_LOG=1',
	'-DCONFIG_PROC_HAVE_CORE_LOG=1',
]

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
warningsGcc = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',
	'-Werror',
	'-Wfatal-errors',
	'-Wreorder',
	'-Wswitch-enum',
	'-Wuseless-cast',
	'-Wparentheses',
	'-Wshift-overflow',
	'-Wsign-compare',
	'-Wzero-as-null-pointer-constant',
	'-Wcast-align',
	'-Wcast-qual',
	'-Wcatch-value',
	'-Wchar-subscripts',
	'-Wswitch-default',
	'-Wctor-dtor-privacy',
	'-Wduplicated-branches',
	'-Wduplicated-cond',
	'-Wempty-body',
	'-Wextra-semi',
	'-Wfloat-equal',
	'-Wformat',
	'-Wformat-extra-args',
	'-Wimplicit-fallthrough',
	'-Wmissing-field-initializers',
	'-Wnull-dereference',
	'-Wshadow',
]

warningsClang = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',
	'-Werror',
	'-Wno-gnu-zero-variadic-macro-arguments',
	'-Wshadow',
	'-Wsign-conversion',
	'-Wnull-dereference',
	'-Wdouble-promotion',
	'-Wimplicit-fallthrough',
	'-Wcast-qual',
	'-Wcast-align',
	'-Wnon-virtual-dtor',
	'-Woverloaded-virtual',
	'-Wfloat-equal',
	'-Wswitch-enum',
	'-Wmissing-declarations',
	'-Wunreachable-code',
	'-Wdocumentation',
	'-Wthread-safety',
]

if host_machine.system() == 'windows'
	args += '-D_WIN32_WINNT=_WIN32_WINNT_WIN10'
	args += '-DWINVER=_WIN32_WINNT_WIN10'

	# wrestool -l CodeOrb.exe
	if cxx.get_id() != 'msvc'
		args += '-D__STDCPP_THREADS__=1'
	endif
else
	args += '-std=gnu++11'
endif

if cxx.get_id() == 'msvc'
	args += '/std:c++17'
elif cxx.get_id() == 'gcc'
	args += warningsGcc
elif cxx.get_id() == 'clang'
	args += warningsClang
endif

# Dependencies

deps = []
deps += dependency('threads')

if host_machine.system() == 'windows'
	deps += cxx.find_library('ws2_32')
endif

# Application

nameExe = 'introducing'
if host_machine.system() == 'windows'
	nameExe = 'Introducing'
endif

myApp = executable(
	nameExe,
	[
		srcs,
	],
	include_directories : include_directories([
		'../..',
	]),
	dependencies : [
		deps,
	],
	cpp_args : [
		args,
	],
)

