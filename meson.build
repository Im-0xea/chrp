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
]
deps = [
  dependency('libcurl'),
  dependency('libsixel'),
  dependency('libxml2'),
]

exe = executable('chrp',
  sources : src,
  dependencies : deps,
  include_directories : inc
)
