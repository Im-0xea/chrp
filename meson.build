project('chrp', 'c')

inc = [
  'include'
]
src = [
  'args.c',
  'search.c',
  'curl_boilerplate.c',

  'pubchem.c',
  'chemspider.c',
]
deps = [
  dependency('libcurl'),
  dependency('libxml2'),
]

exe = executable('chrp',
  sources : src,
  dependencies : deps,
  include_directories : inc
)
