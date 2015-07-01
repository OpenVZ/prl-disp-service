#!/usr/bin/env python

import os
import fnmatch
import sys
import zipfile
import shutil

enable_archive_debug = True

def get_path_files( path, exclude_paths = [], relative = '' ):
    """
    Return the contents of the path - all files that are below this
    Result is a list of pair-list [file_name, file_path]
    """

    entries = []

    # handling normal situatuion with passed folder
    for name in os.listdir( path ):

        valid = 1
        for xf in exclude_paths:
            if os.path.abspath(xf) == os.path.abspath( os.path.join( path, name ) ):
                if enable_archive_debug:
                    print 'Ignored %s ' % os.path.join( path, name )
                valid = 0
                break

        if os.path.isfile( os.path.join( path, name ) ):
            file_name = os.path.normpath( os.path.join( relative, name ) )
            file_path = os.path.abspath( os.path.normpath( os.path.join( path, name ) ) )
            entry = [ file_name, file_path ]
            entries.append( entry )

        elif os.path.isdir( os.path.join( path, name ) ):
            entries += get_path_files( os.path.join( path, name ), exclude_paths, os.path.join( relative, name ) )

    return entries


def archive_path( path, arch, exclude_paths = [], exclude_files = [], include_ext = None, include_only = [], include_anyway = [] ):
    """
    Create archive from the contents of path and place it in the archive named arch

    One can call this as archive_path( './folder/', './folder.zip', ['./floder/ignore_path'], [ '.ncb' ] )
    """

    # Gathering file list to archive
    files = get_path_files( path, exclude_paths, '' )

    # Creating archive file
    archive = zipfile.ZipFile( arch, 'w' )

    # Adding each file to the archive by it's relative name in the folder path
    size = len(files)
    indx = 1
    prev_percent = -1

    current_folder = ''
    current_size = 0
    verbose_mode = sys.argv.count('-v')

    for f in files:

        percent = indx * 100 / size
        if prev_percent != percent and percent % 5 == 0:
            print '%u%% ' % percent,
            prev_percent = percent
            sys.stdout.flush()
        indx += 1

        file_name = f[0]
        file_path = f[1]
        ext = os.path.splitext(file_name)[1]

        need_ignore = 0

        # Searching file in the exclude sequences
        for excf in exclude_files:
            if file_name.endswith( excf ):
                if enable_archive_debug:
                    print 'Ignored %s ...' % file_name
                need_ignore = 1

        if (not include_ext is None) and (not ext in include_ext):
            if enable_archive_debug:
                print 'Ignored %s ...' % file_name
            need_ignore = 1

        if len(include_only) > 0:
            match = 0
            for word in include_only:
                if os.path.basename(file_name).count(word):
                    match = 1
            if not match:
                if enable_archive_debug:
                    print 'Ignored %s ...' % file_name
                need_ignore = 1

        if file_name in include_anyway:
            need_ignore = 0

        if not need_ignore:
            if enable_archive_debug:
                print 'Archiving %s ...' % file_name
            if os.path.islink(file_path):
                # magic with magic numbers for symlinks:
                # http://www.mail-archive.com/python-list@python.org/msg34223.html
                link_dest = os.readlink(file_path)
                attr = zipfile.ZipInfo()
                attr.filename = file_name
                attr.create_system = 3
                attr.external_attr = 2716663808L
                archive.writestr(attr, os.readlink(file_path))
            else:
                #  zipfile.ZIP_DEFLATED - means compression
                #  zipfile.ZIP_STORED - means no compression
                archive.write( file_path, file_name, zipfile.ZIP_DEFLATED )

            if not verbose_mode:
                continue

            file_name = file_path
            file_path = os.path.split(file_name)[0]
            if current_folder != file_path:
                if current_size > 512 * 1024: # printing information about folders > 512KB
                    print 'Archived "%s" ... size %2.2fkb' % (current_folder, current_size / 1024)
                current_folder = file_path
                current_size = 0
            current_size += os.path.getsize(file_name)

    print
    if enable_archive_debug:
        print 'Archive %s created !' % arch


def pack_tests(version):
    # Copying some files to include in python tests archive
    include_only = [ 'test_', 'xml' ]
    archives_dir = '../../z-Build/%s/tests' % version
    tests_archive = '%s/prl-disp-unittests.zip' % archives_dir
    python_tests_archive = '%s/pythontests.zip' % archives_dir

    try:
        if not os.path.exists(archives_dir):
           os.mkdir(archives_dir)
        for root, dirnames, filenames in os.walk('..'):
            for filename in fnmatch.filter(filenames, '*.xml'):
                shutil.copy(os.path.join(root, filename), '../../z-Build/%s' % version)
        archive_path('../../z-Build/%s' % version, tests_archive, include_only=include_only)
        archive_path('../_Python', python_tests_archive)
    except Exception, e:
        print e
        raise RuntimeError('Failed to make archive with unit tests.')

if len(sys.argv) < 2:
    sys.stderr.write('Usage %s [Debug/Release]\n' % sys.argv[0])
    sys.exit(1)

pack_tests(sys.argv[1])
