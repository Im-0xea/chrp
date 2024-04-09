project('chrp', 'c')

inc = [
  'include'
]
src = [
  'args.c',
  'search.c',
  'curl_boilerplate.c',
  'sixel_boilerplate.c',
  'xml_boilerplate.c',

  'pubchem.c',
  'chemspider.c',
  'cas.c',
  'psychonautwiki.c',
]
cc = meson.get_compiler('c')
deps = [
  cc.find_library('m', required : false),
  dependency('libcurl'),
  dependency('cJSON'),
  dependency('libpng'),
  dependency('librsvg-2.0'),
  dependency('libsixel'),
  dependency('libxml2'),
]

exe = executable('chrp',
  sources : src,
  dependencies : deps,
  include_directories : inc
)
