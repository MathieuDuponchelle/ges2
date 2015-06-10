m2m = find_program ('mallard_to_markdown.py')

slate_markdown_pages = custom_target ('docs/slate_markdown_doc_pages/GES.Clip.markdown',
				output : 'GES.Clip.markdown',
				command : [m2m, 'docs/c_doc_pages',
				'-o', 'docs/slate_markdown_doc_pages',
				'-p', 'docs/python_doc_pages/',
				'-i', 'docs/index.markdown',
				'-t', 'slate'],
				depends : [c_doc_pages, python_doc_pages]
			       )
