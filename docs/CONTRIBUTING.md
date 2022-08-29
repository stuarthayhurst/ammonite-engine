# Contributing to Ammonite
## Overview:
  - Your contributions and pull requests are welcome, this project can always use extra help!
  - In short, to contribute:
    - Make an issue describing what you're working on
    - Thoroughly test the contribution
    - Create a merge request, and make any requested changes

## Working with issues:
  - If someone else is already working on a reported issue, feel free help them out. Please don't try and commandeer the issue, if you want to work on something on your own, find another issue
  - Please report large issues before submitting a pull request to fix them
  - If you are working on your own issue, use that report as a space to track information and progress relating to the issue
  - If any help is required, please make it known, instead of silently dropping the issue
    - There's a label to use when help is wanted, to make searching for the issues easier

## General changes:
  - To work on the project, for the repository first, so you can make changes to your copy
  - Each commit should be a meaningful change, and be functional
  - When the changes are ready, submit a pull request, as described in a later section
  - If the changes aren't complete, submit the pull request as a draft instead
  - Changes may be requested, please don't take them personally, they're just to ensure quality and consistency within the project

## Documentation changes:
  - British English should be used in documentation, as well as consistent styling
  - Any new dependencies should be documented under the relevant dependency section
  - Documented information should be updated if the behaviour has changed

## Build system changes:
  - If the behaviour of a target is modified, it should be documented in `README.md`, under "Build system"
  - New build system targets should be documented there, and removed targets removed from there as well
  - New scripts should be placed in `scripts/`, and existing scripts are all located there

## Code changes:
  - The engine must successfully compile after all changes have been made
    - If a demo requires code changes to be made, rebase it on your development branch and make the changes
  - Build system information can be found in `README.md`, under "Build system"
  - Debugging information can be found in `README.md`, under "Debug mode"

## Submitting a pull request:
  - When you believe your contribution to be complete, submit a pull request
    - Follow the template provided when creating a pull request, and fill out relevant information
    - If the code isn't ready to be merged yet, submit the changes as a draft
  - Your changes will be reviewed and either given suggestions for changes, or it'll be approved and merged
  - If possible, please write a summary of changes you made. This makes it easier to make a new release and document the changes

## Other informaton:
  - ALL changes must be allowed under the license (See `LICENSE.md`)
  - ALL changes and discussions must abide by the Code of Conduct (`docs/CODE_OF_CONDUCT.md`)
