"""
This script loads the tests.hxx.jinja template and the data file tests.yaml and 
renders the ready-to-compile tests.hxx class that is read by tester.hpp and 
ultimately provides the data for the unit tests.

Author: Till Blaha 2021
"""

# pip3 install jinja2 jinja2-ansible-filters
from jinja2 import Template, StrictUndefined, Environment, FileSystemLoader
import yaml

# configure jinja2
templateLoader = FileSystemLoader(searchpath="./")
templateEnv = Environment(
    extensions=['jinja2_ansible_filters.AnsibleCoreFiltersExtension'], # this is directly from https://pypi.org/project/jinja2-ansible-filters/
    loader=templateLoader,
    autoescape=True,
    undefined=StrictUndefined,
    )

# load the template files
TEMPLATE_FILE_hxx = "tests.hxx.jinja"
template_hxx = templateEnv.get_template(TEMPLATE_FILE_hxx)
TEMPLATE_FILE_lst = "comm-objects.lst.jinja"
template_lst = templateEnv.get_template(TEMPLATE_FILE_lst)

# open the data file
with open("tests.yaml") as datafile:
    yaml_data = yaml.safe_load(datafile)

# render the template
outputText_hxx = template_hxx.render(yaml_data)
outputText_lst = template_lst.render(yaml_data)

# print to file
with open("tests.hxx", "w") as outfile:
    outfile.write(outputText_hxx)

with open("comm-objects.lst", "w") as outfile:
    outfile.write(outputText_lst)
