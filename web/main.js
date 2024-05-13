document.addEventListener('DOMContentLoaded', (event) => {
	document.querySelector('.search-form')?.addEventListener('submit', e => {
		e.preventDefault();
		search();
	});
});

function search() {
	var searchTerm = document.getElementById("searchInput").value;
	search_raw(searchTerm);
}
function search_raw(searchTerm) {
	console.log("Searching for: " + searchTerm);

	var searchData = { "name": searchTerm };
	console.log("POST: " + JSON.stringify(searchData));

	fetch('http://testhost:234', {
		method: 'POST',
		headers: {
			'Content-Type': 'application/json'
			//'Content-Type': 'text/plain'
		},
		body: JSON.stringify(searchData)
	})
	.then(response => {
		if (!response.ok) {
			throw new Error('Failed to send query');
		}
		return response.json();
	})
	.then(data => {
		console.log('Got results: ', data);

		var columns = 0;
		data.results.forEach(result => {
			columns++;
		});

		var table = document.createElement('table');

		// database name
		var database_name_row = table.insertRow();
		var database_name_cell_p = document.createElement("th");
		database_name_cell_p.innerHTML = "Database";
		database_name_row.appendChild(database_name_cell_p);
		for (var i = 0; i < columns; i++) {
			var database_name_cell = database_name_row.insertCell();
			if (data.results[i].database_name !== undefined) {
				database_name_cell.innerHTML = data.results[i].database_name;
			}
		}

		// structure
		var structure_row = table.insertRow();
		var structure_cell_p = document.createElement("th");
		structure_cell_p.innerHTML = "Structure";
		structure_row.appendChild(structure_cell_p);
		for (var i = 0; i < columns; i++) {
			var structure_cell = structure_row.insertCell();
			if (data.results[i].structure !== undefined) {
				var structure_img = document.createElement('img');
				structure_img.src = data.results[i].structure;
				structure_cell.appendChild(structure_img);
			}
		}

		// id
		var id_row = table.insertRow();
		var id_cell_p = document.createElement("th");
		id_cell_p.innerHTML = "Id";
		id_row.appendChild(id_cell_p);
		for (var i = 0; i < columns; i++) {
			var id_cell = id_row.insertCell();
			if (data.results[i].id !== undefined) {
				id_cell.innerHTML = data.results[i].id;
			}
		}

		// generic name
		var name_row = table.insertRow();
		var name_cell_p = document.createElement("th");
		name_cell_p.innerHTML = "Name";
		name_row.appendChild(name_cell_p);
		for (var i = 0; i < columns; i++) {
			var name_cell = name_row.insertCell();
			if (data.results[i].name_generic !== undefined) {
				name_cell.innerHTML = data.results[i].name_generic;
			}
		}
		
		// systematic name
		var sys_name_row = table.insertRow();
		var sys_name_cell_p = document.createElement("th");
		sys_name_cell_p.innerHTML = "Systematic Name";
		sys_name_row.appendChild(sys_name_cell_p);
		for (var i = 0; i < columns; i++) {
			var sys_name_cell = sys_name_row.insertCell();
			if (data.results[i].name_systematic !== undefined) {
				sys_name_cell.innerHTML = data.results[i].name_systematic;
			}
		}

		// molecular formula
		var form_row = table.insertRow();
		var form_cell_p = document.createElement("th");
		form_cell_p.innerHTML = "Molecular Formula";
		form_row.appendChild(form_cell_p);
		for (var i = 0; i < columns; i++) {
			var form_cell = form_row.insertCell();
			if (data.results[i].molecular_formula !== undefined) {
				form_cell.innerHTML = data.results[i].molecular_formula;
			}
		}

		// molecular weight
		var weight_row = table.insertRow();
		var weight_cell_p = document.createElement("th");
		weight_cell_p.innerHTML = "Molecular Weight";
		weight_row.appendChild(weight_cell_p);
		for (var i = 0; i < columns; i++) {
			var weight_cell = weight_row.insertCell();
			if (data.results[i].molecular_weight !== undefined) {
				weight_cell.innerHTML = data.results[i].molecular_weight;
			}
		}

		// melting points
		var melt_row = table.insertRow();
		var melt_cell_p = document.createElement("th");
		melt_cell_p.innerHTML = "Melting Point(s)";
		melt_row.appendChild(melt_cell_p);
		for (var i = 0; i < columns; i++) {
			var melt_cell = melt_row.insertCell();
			if (data.results[i].melting_points !== undefined) {
				data.results[i].melting_points.forEach(point => {
					melt_cell.innerHTML += point + "<br>";
				});
			}
		}
		
		// boiling points
		var boil_row = table.insertRow();
		var boil_cell_p = document.createElement("th");
		boil_cell_p.innerHTML = "Boiling Point(s)";
		boil_row.appendChild(boil_cell_p);
		for (var i = 0; i < columns; i++) {
			var boil_cell = boil_row.insertCell();
			if (data.results[i].boiling_points !== undefined) {
				data.results[i].boiling_points.forEach(point => {
					boil_cell.innerHTML += point + "<br>";
				});
			}
		}

		document.getElementById('results').innerHTML = "";
		document.getElementById('results').appendChild(table);
	})
	.catch(error => {
		console.error('Failed to query: ', error);
	});
}
