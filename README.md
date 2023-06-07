Project Description

Recently there have been lots of bank robberies. Just a few days ago, the police have identified a possible subject. The subject's motor vehicle contained, among other items, some hard drives (without the laptop). Apparently, the subject had deleted the files without reformatting the drives. Fortunately, the subject had never taken CS 537 before. This means that the subject does not know that most data and indeed most of the file control blocks still reside on the drives.

The police know that bank robbers usually take pictures of banks that they plan to rob. Thus, the police hire you, a file system expert, to be part of the forensics team attempting to reconstruct the contents of the disks. Each disk will be given to you in the form of a disk image (http://en.wikipedia.org/wiki/Disk_imageLinks to an external site. ). A disk image is simply a file containing the complete contents and file system structures (For more, see "Ext2 image file" section below).

To catch the bad guy and prevent future robberies, your goal is to reconstruct all pictures (jpg files only) in the disk images and make the subject regret not taking CS 537 in the past. Of course, you may understand the regret of taking 537.

Once you identify an inode that represents a jpg file, you should copy the content of that file to an output file (stored in your 'output/' directory), using the inode number as the file name. For example, if you detect that inode number 18 is a jpg file, you should create a file 'output/file-18.jpg' which will contain the exact data reachable from inode 18. (See the Example section below for more).

Part 1
First, you need to reconstruct all jpg files (both undeleted and deleted ones) from a given disk image. To do this, you need to scan all inodes that represent regular files and check if the first data block of the inode contains the jpg magic numbers: FF D8 FF E0 or FF D8 FF E1 or FF D8 FF E8 .

Part 2
The second part of your task is to find out the filenames of those inodes that represent the jpg files. Note that filenames are not stored in inodes, but in directory data blocks. Thus, after you get the inode numbers of the jpg files, you should scan all directory data blocks to find the corresponding filenames.

After you get the filename of a jpg file, you should again copy the content of that file to an output file, but this time, using the actual filename. For example, if inode number 18 is 'bank- uwcu.jpg', you should create a file 'output/bank-uwcu.jpg' which will be identical to 'output/file-18.jpg'.

Along with the filename, you should also get some other details about the file including the number of links, the file size, and the Owner's User ID. Create a third file in your output directory with the details in the following format:

links count
file size
owner user id
Name this file file-<inode number>-details.txt, for example, for inode number 18, this file would be 'output/file-18-details.txt'.

When your program starts, your program should create the specified output directory. If the directory already exists, print an error message (any message) and exit the program. You can know if a directory exists by using the opendir(name) (https://linux.die.net/man/3/opendirLinks to an external site.) system call.

To begin, reconstruct all the jpg files from the provided image. We will give you a small image containing a few jpgs to get you started. Later, we will provide a more complex disk image, with secret image files in a folder called "top_secret". This directory can be present at an arbitrarily deep level, so you will need to traverse the directory tree to find it.

In summary, for your final submission of runscan, in your output directory, you should only have the 3 files (both deleted and not deleted) for the jpg files that were in a directory called "top_secret". For each jpg that was in "top_secret", your output directory should contain a file with an inode number as the filename, a file with the same filename as the actual one, and a third file with the details.
