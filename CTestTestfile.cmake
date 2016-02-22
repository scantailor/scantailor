# CMake generated Testfile for 
# Source directory: /Users/pod/scantailor
# Build directory: /Users/pod/scantailor
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ImageProcTests "imageproc/tests/imageproc_tests" "--log_level=message")
add_test(ScanTaylorTests "tests/tests" "--log_level=message")
subdirs(crash_reporter)
subdirs(dewarping)
subdirs(foundation)
subdirs(math)
subdirs(imageproc)
subdirs(interaction)
subdirs(zones)
subdirs(tests)
subdirs(ui)
subdirs(filters/fix_orientation)
subdirs(filters/page_split)
subdirs(filters/deskew)
subdirs(filters/select_content)
subdirs(filters/page_layout)
subdirs(filters/output)
