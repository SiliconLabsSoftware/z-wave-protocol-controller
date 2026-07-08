---
name: generate-new-cc
description: Generate a new command class.
---

# Generate a new command class

In this repository, command class generator is being used to generate the command class skeleton code. The generator is located in the `scripts/command_class_generator` directory, and with cmake the generator is run automatically when the project is configured. For the new command class, the user needs to add the command class to the `config.yaml` file in the `scripts/command_class_generator` directory. After the generator is run, the user needs to implement the command class in the `command_class_<cc-name>` directory. Here the files needs to be modified expect the generated files under the `generated` directory.

## When to use

- Use this skill when the user wants to generate a new command class or modify an existing command class.
- This skill is useful to generate all the boilerplate code for the new command class, such as the attributes, the MQTT interface, the core logic, etc.

## Instructions

1. Read the [command class implementation guide](/docs/command_class_implementation_guide.md) to understand the generated structure and the implementation details.
2. Read the [zwave.xml](/scripts/command_class_generator/zwave.xml) file to understand the command class definition and naming conventions.
3. Add the command class to the `config.yaml` file in the `scripts/command_class_generator` directory.
4. Ask the user to provide information about the command class, such as version, MQTT support, support, control, interview attributes, minimal scheme, etc.
5. Run `cmake --workflow --preset <preset>` to generate the new command class.
6. Implement the command class in the `command_class_<cc-name>` directory.
   - The generated files can determine the attributes to implement, the core logic, the MQTT interface, etc.
   - Implement the required functions in the `cpp` and `hpp` files.
7. Build the project
8. Create documentation for the command class.

## Documentation

- Put the documentation in the `doc` directory of the command class.
- The documentation is markdown format.
- Create a sequence diagram in mermaid format into the documentation file.