m2m = find_program ('mallard_to_markdown.py')

markdown_pages = custom_target ('docs/markdown_doc_pages/GES.Clip.markdown',
				output : 'GES.Clip.markdown',
				command : [m2m, 'docs/c_doc_pages',
				'-o', 'docs/markdown_doc_pages',
				'-p', 'docs/python_doc_pages/',
				'-t', 'markdown'],
				depends : [c_doc_pages, python_doc_pages]
			       )
