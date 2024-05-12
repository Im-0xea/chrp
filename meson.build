project('chrp', 'c')

inc = [
  'include',
  #'subprojects/stb',
]
src = [
  'api.c',
  'args.c',
  'search.c',
  'curl_boilerplate.c',
  'sixel_boilerplate.c',
  'xml_boilerplate.c',

  'pubchem.c',
  'chemspider.c',
  'cas.c',
  'psychonautwiki.c',
  'term.c',
]
cc = meson.get_compiler('c')

deps = [
  cc.find_library('m', required : false),
  dependency('cJSON'),
  dependency('libb64'),
  dependency('libcurl'),
  dependency('libpng'),
  dependency('libsixel'),
  dependency('libxml2'),
]

if get_option('svg_renderer') == 'librsvg'
add_project_arguments('-DSVG_LIBRSVG', language : 'c')
deps += [
  dependency('librsvg-2.0'),
]
elif get_option('svg_renderer') == 'nanosvg'
add_project_arguments('-DSVG_NANOSVG', language : 'c')
inc += [
  'subprojects/stb',
]
deps += [
  dependency('NanoSVG', method : 'cmake', modules : ['NanoSVG::nanosvgrast']),
]
endif

exe = executable('chrp',
  sources : src,
  dependencies : deps,
  include_directories : inc
)
