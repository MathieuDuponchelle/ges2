c_doc_pages = gnome.generate_doc_from_gir(girtargets,
				       	  output_dir : 'c_doc_pages',
  				          dependencies: [gst_player_dep],
				       	  gir_doc_tool_args: ['--online-links',
				       			      '--link-to-gtk-doc',
							      '-lC']
				         )

python_doc_pages = gnome.generate_doc_from_gir(girtargets,
				       	       output_dir : 'python_doc_pages',
  				               dependencies: [gst_player_dep],
				               gir_doc_tool_args: ['--online-links',
				       			           '--link-to-gtk-doc',
							           '-lpython']
				              )

subdir('markdown_doc_pages')
subdir('slate_markdown_doc_pages')
