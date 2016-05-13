# How to contribute

Scan Tailor loves to welcome your contributions. There are several ways to help out:
* Create an [issue](https://github.com/scantailor/scantailor/issues) on GitHub, if you have found a bug
* Write test cases for open bug issues
* Write patches for open bug/feature issues, preferably with test cases included
* Contribute to the [documentation](https://github.com/scantailor/docs)

There are a few guidelines that we need contributors to follow so that we have a
chance of keeping on top of things.

## Getting Started

* Make sure you have a [GitHub account](https://github.com/signup/free).
* Submit an [issue](https://github.com/scantailor/scantailor/issues), assuming one does not already exist.
  * Clearly describe the issue including steps to reproduce when it is a bug.
  * Make sure you fill in the earliest version that you know has the issue.
* Fork the repository on GitHub.

## Making Changes

* Create a topic branch from where you want to base your work.
  * This is usually the master branch.
  * Only target release branches if you are certain your fix must be on that
    branch.
  * To quickly create a topic branch based on master; `git branch
    master/my_contribution master` then checkout the new branch with `git
    checkout master/my_contribution`. Better avoid working directly on the
    `master` branch, to avoid conflicts if you pull in updates from origin.
* Make commits of logical units.
* Check for unnecessary whitespace with `git diff --check` before committing.
* Use descriptive commit messages and reference the #issue number.
* Core test cases should continue to pass and changed/added functions should ship there own test cases. Please check for valid build process before pulling to scantailor/scantailor repository.
* Your work should apply the Scan Tailor coding standards(fit in with the currently used semantics).

## Which branch to base the work

* Bugfix branches will be based on master.
* New features that are backwards compatible will be based on next minor release
  branch.
* New features or other non-BC changes will go in the next major release branch.

## Submitting Changes

* Push your changes to a topic branch in your fork of the repository.
* Submit a pull request to the repository in the Scan Tailor organization, with the
  correct target branch.

## Test cases and codesniffers
* We are currently using boost for testing, so we recommend doing the same.

# Additional Resources

* [Existing issues](https://github.com/scantailor/scantailor/issues)
* [General GitHub documentation](https://help.github.com/)
* [GitHub pull request documentation](https://help.github.com/send-pull-requests/)
* #scantailor IRC channel on freenode.org
