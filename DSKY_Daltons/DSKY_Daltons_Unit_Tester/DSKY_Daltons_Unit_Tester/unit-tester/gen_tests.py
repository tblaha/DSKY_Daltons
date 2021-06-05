"""
This script loads the tests.hxx.jinja template and the data file tests.yaml and 
renders the ready-to-compile tests.hxx class that is read by tester.hpp and 
ultimately provides the data for the unit tests.

Author: Till Blaha 2021
"""

# pip3 install jinja2 jinja2-ansible-filters
from jinja2 import Template, StrictUndefined, Environment, FileSystemLoader
import yaml
import csv

# configure jinja2
templateLoader = FileSystemLoader(searchpath="./")
templateEnv = Environment(
    extensions=['jinja2_ansible_filters.AnsibleCoreFiltersExtension'], # this is directly from https://pypi.org/project/jinja2-ansible-filters/
    loader=templateLoader,
    autoescape=True,
    undefined=StrictUndefined,
    )

# open the data file
with open("tests.yaml") as datafile:
    yaml_data = yaml.safe_load(datafile)

# read variable names from first line in csv (cell A1 is always time, no matter the name)
for i, csvtest in enumerate(yaml_data['csvtests']):
    with open(csvtest['input'], mode='r') as infile:
        reader = csv.reader(infile)
        for line in reader:
            # add variable names defined in the first row of the CSV to the yaml data, then break out the for loop
            yaml_data['csvtests'][i]['variables'] = line[1:]
            break

# load the template files
TEMPLATE_FILES = ["tests.hxx.jinja", "comm-objects.lst.jinja"]
for TEMP_FILE in TEMPLATE_FILES:
    template = templateEnv.get_template(TEMP_FILE)

    # render the template
    outputText = template.render(yaml_data)

    # print to file
    with open(TEMP_FILE.replace(".jinja", ""), "w") as outfile:
        outfile.write(outputText)
