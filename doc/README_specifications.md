# Specification Documentation

**NOTE** The specification is a work in progress and might undergo certain
additions and changes which might not be reflected directly in this document.
If this is the case, an appropriate issue should be opened to signal the
inconsistencies.

This documents provides a description of fields and syntax of the `yaml`
configuration files which are used in conjunction with the metamorphic testing
infrastructure. The file names used here are generic names, but links to
existing examples in the `./config_files` folder should be straightforward. The
files covered are:

* [`config.yaml`](#config)
* [`api_fuzzer.yaml`](#fuzzer-spec)
* [`set_meta_tests.yaml`](#meta-spec)

This document is meant to be comprehensive, covering all possible fields and
values for each specification file. In practice, certain fields will be used
more often than others, and some fields only need be set-up once.

<a name='config' />

### `config.yaml.template`

This file covers path setup information for the main entry point to the
infrastructure, `test_emitter`, and information pertaining to run-time data
for the `meta_runner` script. In order, the following fields are expected to be
provided:

* `working_dir` - an absolute path representing the base directory to use for
  relative path declarations in the file; usually should point to where the tool
  repository resides
* `api_fuzzer_file` - path to the fuzzer specification file
* `meta_test_file` - path to the metamorphic specification file
* `test_emitter_output_file` - relative path to the eventual generated test file
* `meta_runner` - a series of settings to do with the `meta_runner` script
  run-time; these are as follows:
  - `test_emitter_path` - path to the `test_emitter` binary
  - `lib_path` - path to folder containing library files (`.so` or `.a`) for the
    library under test
  - `include_path` - path to folder containing required header files for the
    library under test
  - `lib_build_dir` - path to root library under test build directory
  - `lib_coverage_dir` - TODO
  - `test_compile_dir` - path to use a working directory for the compilation of
    the generated test cases only
  - `test_compile_bin` - path to an executable to use for compiling generated
    test cases (usually a script invoking a compiler with appropriate flags); a
    series of such compilation scripts are provided in `./scripts/compile`
  - `test_source_path` - path to generated test file (generally should be the
    same as `test_emitter_output_file`)
  - `test_run_path` - path to expected test file binary location, after
    compilation (determined by the compilation script used under
    `test_compile_bin`)
  - `default_timeout` - the timeout for each phase of an individual test
    (generation, compilation, execution) in the `meta_runner` script
  - `output_folder` - path to emit all logging files during `meta_runner`
    execution (used as base for all the runner output fields)
  - `log_file_path` - path to comprehensive test logging file
  - `stat_log_file_path` - path to collected statistics log file, based on the
    comprehensive information logged
  - `output_tests_folder` - path where to collect faulty generated test cases
    (i.e. those that failed to compile or complete execution successfully)
  - `coverage_output_dir` - path to save coverage files, if any such files have
    been generated during the testing process

<a name='fuzzer-spec' />

### `api_fuzzer.yaml`

This specification exposes parts of the API of the library under test and allows
the user to also provide some logic in regards to the generation of metamorphic
input variables. The fuzzer specification is used to restrict the search space
of the library under test to only those parts of interest in the library, while
the latter can ensure that the generated metamorphic inputs have certain
features to be exploited in the subsequence metamorphic testing phase.

In the following, the document mentions _comprehensions_. These are pieces of
special syntax which can be used to evaluate to certain objects or values. Thus,
they can be used to provide some control over the fuzzing process (for example,
storing the result of a function call in a particular variable which is reused
later). Available comprehensions are discussed after describing the available
fields.

The file expects the following fields be present, with optional fields marked as
such:

* `includes` - a list of header files to be included in the generated test file;
  these can be header files required for the library under test, or additional
  user-created header files which contain helpful functions to be used in the
  test generation process
* `pre_setup` - a list of source code instructions to emit as provided at the
  beginning of the generated test case; useful for any specific context setup or
  parameter setting; variable declarations can be made here, but it is suggested
  to use `var_decl`, in order to make us of the fuzzer symbol table
* `var_decl` - a list of variables to declare at the beginning of the generated
  test case
  - `name` - name of the variable to generate
  - `type` - type of the variable to generate
  - `func` - [**OPTIONAL**] function used to initialize the variable
  - `value` - [**OPTIONAL**] value to initialize the variable with (generally used
    for objects of primitive type)
* `inputs` - a list of variables given as input to the fuzzer, controlling the
  generation process
  - `name` - name of the input variable
  - `descriptor` - a comprehension describing how to initialize the variable
* `types` - a list of types from the library under test (__Note__ the tool has
  knowledge of primitive types, such as _int_ or _bool_)
  - `name` - type name as defined in the library under test
  - `singleton` - [**OPTIONAL**, defaults to `false`] boolean field which defines
    whether a given type is a singleton; a singleton type will have only a
    single instance generated throughout a test case, and all requests for a
    variable of that type will refer to that one instance
* `funcs` - a list of functions from the library under test; this field is
  populated with what amounts to function signatures, and could be inferred
  directly from source code in the future
  - `name` - name of the function from the library under test
  - `enclosing_class` - [**OPTIONAL**] in the case of member functions, the type of
    the class of which the function is a member of
  - `return_type` - [**OPTIONAL**] the type of the return object, if any
  - `param_types` - [**OPTIONAL**] a list of types representing the parameter types
    expected to be passed to the function, in order
  - `conditions` - [**OPTIONAL**, **UNUSED**] a list of conditions to check for before
    generating a function invocation; if any of the conditions fails, the
    generator will issue an alternative, conservative function invocation
  - `special` - [**OPTIONAL**] a boolean field which controls whether the
    function can be used for fuzzing, or if it is declared only to be explicitly
    used either in the fuzzing generation template or the metamorphic
    specification
  - `static` - [**OPTIONAL**] a boolean field which controls whether a function is
    static or not; currently, this only affects how a function is printed out in
    the test case, and adds some additional checks on each invocation
* `constructors` - a list of basic methods to generate objects of a specified
  type; these functions must be declared in such a way that all required
  dependencies (such as parameter objects of the requested type exist) are met
  at any point
  - `name` - [**OPTIONAL**] the name of the constructor; if it is not given, then
    the name is considered to be the same as the `return_type`
  - `return_type` - the type of the object generated by the constructor
  - `param_types` - a list of expected parameter types to be passed to the
    constructor function, in order
* `set_gen` - a list of groups of fields describing information about
  instructions to generate the eventual metamorphic inputs, such that certain
  properties are known to hold over the generated inputs; note that this
  sequence is independent from one input generation process to another, bar any
  singleton objects, or specially referenced objects
 - `type` - specifies how to interpret further fields; available values are:
   - `func` - a single function invocation should be generated
   - `for` - a sequence of functions should be generated, similar to an
     unrolled `for` loop; this value provides access to a special
     `loop_counter` comprehension variable, and requires the `counter` field be
     given
   - `decl` - a variable declaration, with no initialization, should be generated
   - `seq` - a random sequence of function calls, with no particular special
     control information, should be generated
  - `func` - the name of the function to be invoked
  - `target` - [**OPTIONAL**] if the function is a member function, a comprehension
    indicating which object the function should be called on
  - `return` - [**OPTIONAL**] a comprehension indicating in which object the result
    of the function invocation should be stored
  - `params` - [**OPTIONAL**] a list of comprehensions which indicate which objects
    to pass as parameters to the function invocation; not all parameters must
    be a comprehension, and if a compatible type is passed, then a random
    object of that type will be passed as parameter
  - `counter` - only available for use with a `for` type, expects a range-type
    field with integer boundaries (i.e. `[min,max]`); iterates over the range
    with a step of 1, and emits a corresponding function invocation for each
    step

###### Comprehensions (fuzzer)

Comprehensions are pieces of special syntax in the specification files which can
be used to control the random choices made throughout the generation process.
For instance, one good approach is to ensure that at some point in the `set_gen`
sequence, the `return` field of an instruction group is set to
`<special=output_var=>`, which evaluated to the variable representing the
corresponding memtamorphic input being generated. Another usage pattern is
ensuring that certain function parameters are _new_, to further explore the
fuzzing space.

The form of a comprehensions usually comprises of a front (`<`) and back (`>`)
delimiter, with two instances of a middle (`=`) delimiter. This provides three
fields which are used to describe the object to be referenced:
`<type=method=descriptor>`. The `type` field describes what the comprehension
should produce, such as a variable or a primitive value. The `method` field
selects how to generate the requested object; for instance, a `new` method says
that a new object of the given type must be generated, with all the appropriate
recursive fuzzing. Finally, the `descriptor` field further controls the `method`
field, and is essentially an argument to the latter.

The following comprehensions are provided, split by `method`, then by `type`:

* `var` - the result should be a variable
  - `name` - retrieve the variable with the name as in the `descriptor`
  - `type` - retrieve a variable of the type as given in the `descriptor`
  - `new` - build a new variable of the type given in the `descriptor` and
    use that new reference
  - `id` - retrieve a variable by the internal id as given in the `descriptor`
  - `input` - retrieve the value of the input variable with the name given in
    the `descriptor`
* `meta` - **WIP**
  - `input`
  - `var`
* `special` - comprehensions which refer to special, potentially scope-limited
  objects or values
  - `output_var` - return a reference to the currently generated metamorphic input
  - `loop_counter` - only available in the `set_gen` dictionary and under a
    `for` type entry, returns the current value of the counter, which is of
    primitive type `unsigned int`
  - `var_name` - return a primitive type `string` value representing a variable
    name, which is guaranteed to be a unique string value; the form is
    `var_id`, where `id` is a unique numerical identifier
* primitive comprehensions have the type of the returned primitive value, such
  as `int` or `string`; the result will be a value of the respective primitive
  type, not a variable containing the result
  - `val` - return a value of the requested type with the value as given in the
    descriptor
  - `len` - return a value with length as given in the descriptor
  - `random` - return a completely random value of the requested type

<a name='meta-spec' />

### `meta_tests.yaml`

The metamorphic specification specifies certain parameters regarding the
construction of the generated test case, in addition to defining the metamorphic
relations that are used during the metamorphic tests (or more concretely, the
metamorphic variant generation).

###### Test parameters

The following fields indicate certain parameters of the test case to be
generated.

* `input_count` - an integer representing the number of total metamorphic inputs
  to be generated by the fuzzer; it is expected to be at least equal to the
  value of the greatest metamorphic input comprehension value used (see
  [here](#comprehensions-meta))
* `meta_var_type` - a string representing the name of the type of the
  metamorphic inputs and metamorphic variants; at the time, intermediate values
  of the metamorphic variants must also be of the same time
* `meta_test_count` - the number of metamorphic relations to apply in order to
  generate one metamorphic variant
* `variant_count` - the number of total metamorphic variants to generate; note
  that identical variants (e.g. variants for which the sequence of concrete
  relations are identical) are not allowed, therefore this acts as an upper
  limit to the number of variants generated
* `meta_check` - a list of code snippets representing what conditions to check
  for equivalence between the generated metamorphic variants

###### Generators

The following fields declare the metamorphic relations. In addition to the usual
usage of these relations, we also provide support for *generators*, which are
instructions which should generate objects with very specific properties on
demand. For instance, *identity elements* (e.g. 0 or 1 in an integer domain, the
empty and the universe set in a set domain) are very powerful tools and open up
further metamorphic relations to be defined.

Generators should be defined via the `generators` field. Each entry in the field
should be a dictionary, where the key is a representative name for that
generator. The value should be two further fields: `identifier` should be a
letter (except for `m`) to be used for referring to the declared generator via
comprehensions, while `relations` should be a list containing strings
representing instructions from the API of the library under test which all yield
objects with the corresponding property as defined by the generator.

###### Metamorphic relations

The core component of the metamorphic specification remains the metamorphic
relation declaration. We split these relations into *families*, such that all
relations within the same family should yield the same output given the same
inputs. We note that practical testing should be able to identify if any
relations provide violate the previous condition; the check should fail and
further inspection should show that it was not due to a library fault.

In order to define relations, the `relations` field is expected to be provided.
The value passed should be a dictionary, where the key is a string representing
the name of the family relation (the name has no meaning in the generator
itself), while the value is a list of strings representing a single line
instruction in the API of the library under test.

<a name='comprehensions-meta' />

###### Comprehensions (metamorphic)

Metamorphic comprehensions are used to refer to certain variables in relations
and generators. Each comprehension is a string starting with a `%` symbol,
followed by usually a one-character identifier, but sometimes more. We
distinguish the following categories of values we can refer to:

* metamorphic variants - each generated test case will produce a number of
  semantically equivalent by construction variants, over which we would like to
  define some checks to ensure that the variants indeed have the expected
  properties. In order to refer to a variant, the identifier used is `m`.
  Furthermore, we distinguish the _current metamorphic variant being produced_,
  which can be retrieved by `%m`, and the specific metamorphic variant which was
  the *n*th produced, retrieve by `%mn`, where *n* is an integer. In practice,
  `%m0` is expected to be used overwhelmingly in metamorphic checks for
  pair-wise equality.
* metamorphic inputs - generally these will be represented the variables
  produced by the fuzzer, but it might be the case that the variables are loaded
  in from existing data. No matter the origin, we expect there to be a number of
  readily available variable of the corresponding `meta_var_type` type available
  for the metamorphic test generator to use. These can be retrieved by passing a
  number to the comprehension. Thus, `%5` would expect there to be at least 5
  inputs available, and would pick the fifth input given chronologically.
* generators - user-defined generators can be used at practically any point to
  replace a variable, which we know would have a specific property. They are
  retrieved by passing the user-defined generator identifier, currently limited
  to a single alphabetical character. The comprehension will expand to an inline
  instruction used to generate the required object.  
