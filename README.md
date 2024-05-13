# Chrp

### Usage

~~~
$ chrp dihydrogen oxide
Searching for: dihydrogen oxide...

PubChem:
<sixel of molecular structure>
Id: 962
Name: Water
Systematic IUPAC name: oxidane
Molecular Formula: H2O
Molecular Weight: 18.015
...
~~~
##### API
~~~
$ chrp --mode=web --port=234
address: *.*.*.*
port: 234
threads: 4
received request: { "name": "dihydrogen oxide" }
reply send

curl -X POST -d "{ \"name\": \"dihydrogen oxide\" }" http://localhost:234 
{
	"results":	[{
			"database_name":	"PubChem",
			"structure_type":	"PNG",
			"structure":	<base64 encoding of png file>,
			"id":	"962",
			"name_generic":	"Water",
			"name_systematic":	"oxidane",
			"molecular_formula":	"H2O",
			"molecular_weight":	"18.015",
			...
		},
		...
	]
}
~~~
