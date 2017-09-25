from distutils.core import setup

setup(name = 'gscope',
      version = '0.2',
      description = 'Simple GTK forntend for cscope',
      author = 'Cyrill Gorcunov',
      author_email = 'gorcunov@gmail.com',
      url = 'https://github.com/cyrillos/gscope',
      download_url = 'https://github.com/cyrillos/gscope/archive/v0.2.tar.gz',
      packages = ['gscope'],
      package_dir = {'gscope' : 'src'},
      package_data = {'gscope': ['data/*']},
      scripts = ['src/gscope'],
      )
