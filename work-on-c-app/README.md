# DOCUMENTATION FOR NGINX UNIT C MODULE

	Copyright (C) 2023 Atle Solbakken

## Notes

These files are for initial testing during development.
Please delete them when testing framework may be used instead.
The documentation in this file is to be moved to other more suitable places.

## Preface

The C application for NGINX Unit allows handling requests using C or C++ code.
Modules are compiled as needed when the app starts or reloads.

## How it works

The module file to compile is specified in the configuration along with compiler flags.

When the C app starts, the compiler is started in the background.
There is no immediate feedback of the compilation step until the first request is made to the app.
If the compilation is slow, the clients will receive 503 errors while the compiler runs.

Compiling and linking is performed in a single step (the compiler is expected to call the linker).
The compiler must return status code 0 when the compilation succeeds.

Implicitly, the compiler flag **-shared** will always be added (to build a shared library) along with the **-o**
output specifier and file names.
All compiler flags are logged when the compiler starts.

The app will not restart or fail fatally unless it cannot start the compiler.
If there are any problems with the file being compiled, the errors will be visible in the Unit log files.
The app will try to compile the file again and re-check the result as requests are being received.
During development, it is thus possible to observe errors in the Unit log files and simply send requests
to the app to force the compiler to run again.

## Configuration options for the C application

| Parameter                    | Value                                                                     |
|------------------------------|---------------------------------------------------------------------------|
| type                         | Always set to **c**                                                       |
| working\_directory           | Where the application is located.                                         |
| prefix                       | Name of the module to compile. The file **{prefix}**.c in the working dir will be compiled to **{prefix}**.o and then loaded |
| cc                           | The compiler command to use, defaults to **clang**.                       |
| flags                        | An array of compiler flags.

### Example configuration

The module **blah**, which is expected to be found in **blah.c**
is started and receives requests on port **7080**.

The compiler **clang** is used to produce the shared object **blah.o**.

The compiler flags set here are the recommended minimum flags.

	{
	  "listeners": {"*:7080": {"pass": "applications/blah"}},
	  "applications": {
	    "blah": {
	      "type": "c",
	      "working_directory": "./",
	      "prefix": "./blah",
	      "cc": "/usr/bin/clang",
	      "flags": [
	        "-B/usr/bin",
	        "-I../src",
	        "-I../build/include",
	        "-Wall",
	        "-Wextra",
	        "-Werror"
	      ]
	    }
	  }
	}

## C module

The module must provide the symbol **nxt_c_module** of type **nxt_c_mod_t**.
A request handler must be set in the struct.

The handler receives a native Unit request (**nxt_unit_request_info_t**) and
should handle this accordingly.

Either **NXT_UNIT_OK** or **NXT_UNIT_ERROR** may be returned from the handler.
Returning an error has no other effect than producing an error to the client.

The handler **must not** call **nxt_unit_request_done()**, this is done in the
application.

As of now, the handler will only receive requests from one thread.

### Example module

This example module will produce 200 responses for all requests.

```c
#include <c/nxt_c.h>

static nxt_int_t nxt_c_blah_request_handler(nxt_unit_request_info_t *req);

NXT_EXPORT nxt_c_mod_t nxt_c_module = {
    .request_handler = nxt_c_blah_request_handler,
};


static nxt_int_t
nxt_c_blah_request_handler(nxt_unit_request_info_t *req)
{
    (void)(req);
    nxt_unit_debug(req->ctx, "blah request handler");
    return NXT_UNIT_OK;
}
```
