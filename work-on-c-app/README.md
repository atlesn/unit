# DOCUMENTATION FOR NGINX UNIT C MODULE

## Preface

These files are for initial testing during development.
Please delete them when testing framework may be used instead.
The documentation in this file is to be moved to other more suitable places.

## Rationale

Create applications for NGINX Unit using C or C++.
The apps are compiled on demand when Unit starts the C module.

## Configuration options for application

| Parameter                    | Value                                                                     |
|------------------------------|---------------------------------------------------------------------------|
| type                         | Always set to **c**                                                       |
| working\_directory           | Where the application is located.                                         |
| name                         | Name of the application. The file **{name}**.c in the working dir will be compiled to **{name}**.o and then loaded |
| cc                           | The compiler command to use, defaults to **clang**.                       |


