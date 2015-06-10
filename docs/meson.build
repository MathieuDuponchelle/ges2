sections_file = '@0@/@1@'.format(meson.current_source_dir(), 'ges-sections.txt')

c_doc_pages = gnome.generate_doc_from_gir(girtargets,
				       	  output_dir : 'c_doc_pages',
  				          dependencies: [gst_player_dep],
					  link_to_gtk_doc: true,
					  online_links : true,
					  sections_file : sections_file,
					  language : 'C'
				         )

python_doc_pages = gnome.generate_doc_from_gir(girtargets,
				       	       output_dir : 'python_doc_pages',
  				               dependencies: [gst_player_dep],
					       link_to_gtk_doc: true,
					       online_links : true,
					       sections_file : sections_file,
					       language : 'python'
				              )

subdir('markdown_doc_pages')
subdir('slate_markdown_doc_pages')

sh = find_program ('sh')
update_slate = find_program ('update_slate.sh')
slate_dir = '@0@/@1@'.format(meson.current_source_dir(), 'slate')
built_slate_dir = '@0@/@1@'.format(meson.current_build_dir(), 'slate')
markdown_pages = '@0@/@1@'.format(meson.current_build_dir(), 'slate_markdown_doc_pages')
index_markdown = '@0@/@1@'.format(meson.current_build_dir(), 'index.markdown')

slate = custom_target ('slate.stamp',
		       output : 'slate.stamp',
		       command : [sh, update_slate, built_slate_dir,
		       		  slate_dir, markdown_pages, index_markdown],
		       depends : slate_markdown_pages,
		      )
