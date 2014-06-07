#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process Android resources to generate R.java, and prepare for packaging.

This will crunch images and generate v14 compatible resources
(see generate_v14_compatible_resources.py).
"""

import optparse
import os
import re
import shlex
import shutil

import generate_v14_compatible_resources

from util import build_utils

def ParseArgs():
  """Parses command line options.

  Returns:
    An options object as from optparse.OptionsParser.parse_args()
  """
  parser = optparse.OptionParser()
  parser.add_option('--android-sdk', help='path to the Android SDK folder')
  parser.add_option('--android-sdk-tools',
                    help='path to the Android SDK build tools folder')
  parser.add_option('--R-dir', help='directory to hold generated R.java')
  parser.add_option('--dependencies-res-dirs',
                    help='directories containing resources to be packaged')
  parser.add_option('--resource-dir',
                    help='directory containing this target\'s resources.')
  parser.add_option('--crunch-output-dir',
                    help='directory to hold crunched resources')
  parser.add_option('--non-constant-id', action='store_true')
  parser.add_option('--custom-package', help='Java package for R.java')
  parser.add_option('--android-manifest', help='AndroidManifest.xml path')
  parser.add_option('--proguard-file',
                    help='Path to proguard.txt generated file')

  parser.add_option('--res-v14-compatibility-dir',
                    help='output directory into which '
                         'v14 compatible resources will be generated')
  parser.add_option(
      '--v14-verify-only',
      action='store_true',
      help='Do not generate v14 resources. Instead, just verify that the '
      'resources are already compatible with v14, i.e. they don\'t use '
      'attributes that cause crashes on certain devices.')

  parser.add_option(
      '--extra-res-packages',
      help='Additional package names to generate R.java files for')
  parser.add_option(
      '--extra-r-text-files',
      help='For each additional package, the R.txt file should contain a '
      'list of resources to be included in the R.java file in the format '
      'generated by aapt')

  parser.add_option('--stamp', help='File to touch on success')

  (options, args) = parser.parse_args()

  if args:
    parser.error('No positional arguments should be given.')

  # Check that required options have been provided.
  required_options = (
      'android_sdk',
      'android_sdk_tools',
      'android_manifest',
      'dependencies_res_dirs',
      'resource_dir',
      'crunch_output_dir',
      'R_dir',
      'res_v14_compatibility_dir',
      )
  build_utils.CheckOptions(options, parser, required=required_options)

  return options


def CreateExtraRJavaFiles(
    r_dir, extra_packages, extra_r_text_files):
  if len(extra_packages) != len(extra_r_text_files):
    raise Exception('--extra-res-packages and --extra-r-text-files'
                    'should have the same length')

  java_files = build_utils.FindInDirectory(r_dir, "R.java")
  if len(java_files) != 1:
    return
  r_java_file = java_files[0]
  r_java_contents = open(r_java_file).read()

  for package in extra_packages:
    package_r_java_dir = os.path.join(r_dir, *package.split('.'))
    build_utils.MakeDirectory(package_r_java_dir)
    package_r_java_path = os.path.join(package_r_java_dir, 'R.java')
    open(package_r_java_path, 'w').write(
        re.sub(r'package [.\w]*;', 'package %s;' % package, r_java_contents))
    # TODO(cjhopman): These extra package's R.java files should be filtered to
    # only contain the resources listed in their R.txt files. At this point, we
    # have already compiled those other libraries, so doing this would only
    # affect how the code in this .apk target could refer to the resources.



def MoveImagesToNonMdpiFolders(res_root):
  """Move images from drawable-*-mdpi-* folders to drawable-* folders.

  Why? http://crbug.com/289843
  """
  for src_dir_name in os.listdir(res_root):
    src_components = src_dir_name.split('-')
    if src_components[0] != 'drawable' or 'mdpi' not in src_components:
      continue
    src_dir = os.path.join(res_root, src_dir_name)
    if not os.path.isdir(src_dir):
      continue
    dst_components = [c for c in src_components if c != 'mdpi']
    assert dst_components != src_components
    dst_dir_name = '-'.join(dst_components)
    dst_dir = os.path.join(res_root, dst_dir_name)
    build_utils.MakeDirectory(dst_dir)
    for src_file_name in os.listdir(src_dir):
      if not src_file_name.endswith('.png'):
        continue
      src_file = os.path.join(src_dir, src_file_name)
      dst_file = os.path.join(dst_dir, src_file_name)
      assert not os.path.lexists(dst_file)
      shutil.move(src_file, dst_file)


def DidCrunchFail(returncode, stderr):
  """Determines whether aapt crunch failed from its return code and output.

  Because aapt's return code cannot be trusted, any output to stderr is
  an indication that aapt has failed (http://crbug.com/314885), except
  lines that contain "libpng warning", which is a known non-error condition
  (http://crbug.com/364355).
  """
  if returncode != 0:
    return True
  for line in stderr.splitlines():
    if line and not 'libpng warning' in line:
      return True
  return False


def main():
  options = ParseArgs()
  android_jar = os.path.join(options.android_sdk, 'android.jar')
  aapt = os.path.join(options.android_sdk_tools, 'aapt')

  build_utils.DeleteDirectory(options.R_dir)
  build_utils.MakeDirectory(options.R_dir)

  generate_v14_compatible_resources.GenerateV14Resources(
      options.resource_dir,
      options.res_v14_compatibility_dir,
      options.v14_verify_only)

  # Generate R.java. This R.java contains non-final constants and is used only
  # while compiling the library jar (e.g. chromium_content.jar). When building
  # an apk, a new R.java file with the correct resource -> ID mappings will be
  # generated by merging the resources from all libraries and the main apk
  # project.
  package_command = [aapt,
                     'package',
                     '-m',
                     '-M', options.android_manifest,
                     '--auto-add-overlay',
                     '-I', android_jar,
                     '--output-text-symbols', options.R_dir,
                     '-J', options.R_dir]
  all_res_dirs = ([options.resource_dir]
                  + shlex.split(options.dependencies_res_dirs))
  for res_dir in all_res_dirs:
    package_command += ['-S', res_dir]
  if options.non_constant_id:
    package_command.append('--non-constant-id')
  if options.custom_package:
    package_command += ['--custom-package', options.custom_package]
  if options.proguard_file:
    package_command += ['-G', options.proguard_file]

  build_utils.CheckOutput(package_command)

  if options.extra_res_packages:
    CreateExtraRJavaFiles(
        options.R_dir,
        build_utils.ParseGypList(options.extra_res_packages),
        build_utils.ParseGypList(options.extra_r_text_files))

  # Crunch image resources. This shrinks png files and is necessary for 9-patch
  # images to display correctly.
  build_utils.DeleteDirectory(options.crunch_output_dir)
  build_utils.MakeDirectory(options.crunch_output_dir)
  aapt_cmd = [aapt,
              'crunch',
              '-S', options.resource_dir,
              '-C', options.crunch_output_dir]
  build_utils.CheckOutput(aapt_cmd, fail_func=DidCrunchFail)

  MoveImagesToNonMdpiFolders(options.crunch_output_dir)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  main()
