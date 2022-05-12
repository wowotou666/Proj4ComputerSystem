#define FUSE_USE_VERSION 26

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>

#include "cs1550.h"

#define MIN(A,B) ((A) <= (B)? (A) :(B))

/**
 * my functions
 */ 
static void writeb(size_t bno, void *buf);
static void readb(size_t bno, void *buf);
static int dir_exist(const char *dir);
static int file_exist(size_t bno, const char *filename, const char *extension);
static int file_size(size_t bno, const char *filename, const char *extension);

FILE *fptr;

/**
 * write a block
 */ 
static void writeb(size_t bno, void *buf) {
	fseek(fptr, bno * BLOCK_SIZE, SEEK_SET);
	fwrite(buf, 1, BLOCK_SIZE, fptr);
}

/**
 * read a block
 */ 
static void readb(size_t bno, void *buf) {
	fseek(fptr, bno * BLOCK_SIZE, SEEK_SET);
	fread(buf, 1, BLOCK_SIZE, fptr);
}

/**
 * Determines whether the dir is existed
 */ 
static int dir_exist(const char *dir) {
	struct cs1550_root_directory rd;
	readb(0, &rd);
	size_t i;
	for (i = 0; i < rd.num_directories; i++) {
		if (strcmp(dir, rd.directories[i].dname) == 0) {
			return rd.directories[i].n_start_block;
		}
	}
	return -1;
}

/**
 * Determines whether the file is included
 */
static int file_exist(size_t bno, const char *filename, const char *extension) {
	struct cs1550_directory_entry d;
	readb(bno, &d);
	size_t i;
	for (i = 0; i < d.num_files; i++) {
		if (strcmp(filename, d.files[i].fname) == 0 && strcmp(extension, d.files[i].fext) == 0) {
			return 1;
		}
	}
	return -1;
}


/**
 * get the file size
 */ 
static int file_size(size_t bno, const char *filename, const char *extension) {
	struct cs1550_directory_entry d;
	readb(bno, &d);
	size_t i;
	for (i = 0; i < d.num_files; i++) {
		if (strcmp(filename, d.files[i].fname) == 0 && strcmp(extension, d.files[i].fext) == 0) {
			return d.files[i].fsize;
		}
	}
	return -1;
}


/**
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not.
 *
 * `man 2 stat` will show the fields of a `struct stat` structure.
 */
static int cs1550_getattr(const char *path, struct stat *statbuf)
{

	// Clear out `statbuf` first -- this function initializes it.
	memset(statbuf, 0, sizeof(struct stat));

	// Check if the path is the root directory.
	if (strcmp(path, "/") == 0) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;

		return 0; // no error
	}

	// Check if the path is a subdirectory.
	if (strchr(path + 1, '/') == NULL && dir_exist(path + 1) != -1) {
		statbuf->st_mode = S_IFDIR | 0755;
		statbuf->st_nlink = 2;

		return 0; // no error
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	// Check if the path is a file.
	int bno = dir_exist(directory);
	if (file_exist(bno, filename, extension) == 1) {
		// Regular file
		statbuf->st_mode = S_IFREG | 0666;

		// Only one hard link to this file
		statbuf->st_nlink = 1;

		// File size -- replace this with the real size
		statbuf->st_size = file_size(bno, filename, extension);

		return 0; // no error
	}

	// Otherwise, the path doesn't exist.
	return -ENOENT;
}

/**
 * Called whenever the contents of a directory are desired. Could be from `ls`,
 * or could even be when a user presses TAB to perform autocompletion.
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	// Add the subdirectories or files.
	// The +1 hack skips the leading '/' on the filenames.
	if (strcmp("/", path) == 0) {

		// The filler function allows us to add entries to the listing.
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		// add subdirectories
		struct cs1550_root_directory rd;
		readb(0, &rd);
		size_t i;
		for (i = 0; i < rd.num_directories; i++) {
			filler(buf, rd.directories[i].dname, NULL, 0);
		}
		return 0;
	} else if (strchr(path + 1, '/') == NULL) {
		// is a sub-dir
		// The filler function allows us to add entries to the listing.
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		// get the block number of sub-dir
		int bno = dir_exist(path + 1);
		if (bno == -1) {
			return -ENOENT;
		}

		// read the sub-dir from disk
		struct cs1550_directory_entry sb;
		readb(bno, &sb);

		size_t i;
		for (i = 0; i < sb.num_files; i++) {
			char name[MAX_FILENAME + MAX_EXTENSION + 2];
			strcpy(name, sb.files[i].fname);
			name[strlen(sb.files[i].fname)] = '.';
			strcpy(name + strlen(sb.files[i].fname) + 1, sb.files[i].fext);
			name[strlen(sb.files[i].fname) + strlen(sb.files[i].fext) + 1] = '\0';
			filler(buf, name, NULL, 0);
		}
		return 0;
	} else {
		return -ENOTDIR;
	}
}

/**
 * Creates a directory. Ignore `mode` since we're not dealing with permissions.
 */
static int cs1550_mkdir(const char *path, mode_t mode)
{
	
	(void) mode;

	if (strlen(path + 1) > MAX_FILENAME) {
		return -ENAMETOOLONG;
	}

	if (strchr(path + 1, '/') != NULL) {
		return -EPERM;
	}

	struct cs1550_root_directory rd;
	readb(0, &rd);

	// already exists
	size_t i;
	for (i = 0; i < rd.num_directories; i++) {
		if (strcmp(rd.directories[i].dname, path+1) == 0) {
			return -EEXIST;
		}
	}

	if (rd.num_directories >= MAX_DIRS_IN_ROOT) {
		return -EPERM;
	}

	// add sub-dir
	++rd.last_allocated_block;
	rd.directories[rd.num_directories].n_start_block = rd.last_allocated_block;
	strcpy(rd.directories[rd.num_directories].dname, path + 1);
	rd.num_directories++;
	
	// update the .disk
	writeb(0, &rd);

	return 0;
}

/**
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{

	(void) path;

	return 0;
}

/**
 * Does the actual creation of a file. `mode` and `dev` can be ignored.
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{

	(void) mode;
	(void) dev;

	if (strchr(path + 1, '/') == NULL) {
		return -EPERM;
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	if (strlen(filename) > MAX_FILENAME || strlen(extension) > MAX_EXTENSION) {
		return -ENAMETOOLONG;
	}

	// read the sub-dir from disk
	int bno = dir_exist(directory);
	if (bno == -1) {
		return -1;
	}
	struct cs1550_directory_entry sd;
	readb(bno, &sd);
	
	// already exists
	size_t i;
	for (i = 0; i < sd.num_files; i++) {
		if (strcmp(filename, sd.files[i].fname) == 0 && strcmp(extension, sd.files[i].fext) == 0) {
			return -EEXIST;
		}
	}

	if (sd.num_files == MAX_FILES_IN_DIR) {
		return -ENOSPC;
	}

	// add file
	struct cs1550_root_directory rd;
	readb(0, &rd);
	rd.last_allocated_block++;
	
	sd.files[sd.num_files].fsize = 0;
	strcpy(sd.files[sd.num_files].fext, extension);
	strcpy(sd.files[sd.num_files].fname, filename);
	sd.files[sd.num_files].n_index_block = rd.last_allocated_block;

	// init file
	struct cs1550_index_block ib;
	memset(&ib, 0, sizeof(ib));
	rd.last_allocated_block++;
	ib.entries[0] = rd.last_allocated_block;
	writeb(sd.files[sd.num_files].n_index_block, &ib);

	
	// file num plus one
	sd.num_files++;

	// wirte back
	writeb(0, &rd);
	writeb(bno, &sd);

	return 0;
}

/**
 * Deletes a file.
 */
static int cs1550_unlink(const char *path)
{

	(void) path;

	return 0;
}

/**
 * Read `size` bytes from file into `buf`, starting from `offset`.
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
		       struct fuse_file_info *fi)
{	

	(void) fi;

	// is a dir
	if (strchr(path + 1, '/') == NULL || strcmp("/", path) == 0) {
		return -EISDIR;
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	// sub-dir not found
	int bno = dir_exist(directory);
	if (bno == -1) {
		return -ENOENT;
	}

	// file not found
	struct cs1550_directory_entry sd;
	readb(bno, &sd);
	size_t i, flag;
	flag = 0;
	for (i = 0; i < sd.num_files; i++) {
		if (strcmp(filename, sd.files[i].fname) == 0 && strcmp(extension, sd.files[i].fext) == 0) {
			flag = 1;
			break;
		}
	}
	if (flag == 0) {
		return -ENOENT;
	}

	// read the index_block
	struct cs1550_index_block ib;
	struct cs1550_file_entry fe = sd.files[i];
	readb(fe.n_index_block, &ib);

	size_t fsize = sd.files[i].fsize;

	// there is no data to read
	if (offset >= (off_t)fsize) {
		return 0;
	}

	if (size > fsize || size > (fsize - offset)) {
		size = (fsize - offset);
	}

	size_t pos = 0;
	char buffer[BLOCK_SIZE];
	while (pos < size) {
		readb(ib.entries[(pos + offset) / BLOCK_SIZE], buffer);
		memcpy(buf + pos, buffer + (offset + pos) % BLOCK_SIZE, MIN((BLOCK_SIZE - (offset + pos) % BLOCK_SIZE), (size - pos)));
		pos += MIN((BLOCK_SIZE - (offset + pos) % BLOCK_SIZE), (size - pos));
	}

	return size;
}

/**
 * Write `size` bytes from `buf` into file, starting from `offset`.
 */
static int cs1550_write(const char *path, const char *buf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{

	(void) fi;

	// is a dir
	if (strchr(path + 1, '/') == NULL || strcmp("/", path) == 0) {
		return -EISDIR;
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	// sub-dir not found
	int bno = dir_exist(directory);
	if (bno == -1) {
		return -ENOENT;
	}

	// file not found
	struct cs1550_directory_entry sd;
	readb(bno, &sd);
	size_t i, flag;
	flag = 0;
	for (i = 0; i < sd.num_files; i++) {
		if (strcmp(filename, sd.files[i].fname) == 0 && strcmp(extension, sd.files[i].fext) == 0) {
			flag = 1;
			break;
		}
	}
	if (flag == 0) {
		return -ENOENT;
	}

	// read the index_block
	struct cs1550_index_block ib;
	struct cs1550_file_entry *fe = &sd.files[i];
	readb(fe->n_index_block, &ib);

	// read the root-dir
	struct cs1550_root_directory rd;
	readb(0, &rd);

	// Prevent exceeding the maximum size
	if (offset + size > MAX_ENTRIES_IN_INDEX_BLOCK * BLOCK_SIZE) {
		size = (MAX_ENTRIES_IN_INDEX_BLOCK * BLOCK_SIZE) - offset;
	}

	size_t nsize = offset + size;
	for (i = fe->fsize; (i / BLOCK_SIZE) < (nsize / BLOCK_SIZE); i += BLOCK_SIZE) {
		ib.entries[(i / BLOCK_SIZE) + 1] = ++rd.last_allocated_block;
	}

	fe->fsize = offset + size;

	i = 0;
	char buffer[BLOCK_SIZE];
	while (i < size) {
		readb(ib.entries[(i + offset) / BLOCK_SIZE], buffer);
		uint32_t c = MIN((size - i), (BLOCK_SIZE - ((i + offset) % BLOCK_SIZE)));
		memcpy(buffer + ((i + offset) % BLOCK_SIZE), buf + i, c);
		writeb(ib.entries[(i + offset) / BLOCK_SIZE], buffer);
		i += c;
	}


	writeb(0, &rd);
	writeb(fe->n_index_block, &ib);
	writeb(bno, &sd);

	return size;
}

/**
 * Called when a new file is created (with a 0 size) or when an existing file
 * is made shorter. We're not handling deleting files or truncating existing
 * ones, so all we need to do here is to initialize the appropriate directory
 * entry.
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;

	return 0;
}

/**
 * Called when we open a file.
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) fi;

	// If we can't find the desired file, return an error
	if (strcmp("/", path) == 0) {
		return 0;
	}

	if (strchr(path + 1, '/') == NULL && dir_exist(path + 1) != -1) {
		return 0;
	}

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

	if (dir_exist(directory) != -1 && file_exist(dir_exist(directory), filename, extension) != -1) {
		return 0;
	}

	return -ENOENT;

	// Otherwise, return success
	// return 0;
}

/**
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush(const char *path, struct fuse_file_info *fi)
{

	(void) path;
	(void) fi;

	// Success!
	return 0;
}

/**
 * This function should be used to open and/or initialize your `.disk` file.
 */
static void *cs1550_init(struct fuse_conn_info *fi)
{
	(void) fi;

	// Add your initialization routine here.
	fptr = fopen(".disk", "rb+");
	return NULL;
}

/**
 * This function should be used to close the `.disk` file.
 */
static void cs1550_destroy(void *args)
{

	(void) args;

	// Add your teardown routine here.
	fclose(fptr);
}

/*
 * Register our new functions as the implementations of the syscalls.
 */
static struct fuse_operations cs1550_oper = {
	.getattr	= cs1550_getattr,
	.readdir	= cs1550_readdir,
	.mkdir		= cs1550_mkdir,
	.rmdir		= cs1550_rmdir,
	.read		= cs1550_read,
	.write		= cs1550_write,
	.mknod		= cs1550_mknod,
	.unlink		= cs1550_unlink,
	.truncate	= cs1550_truncate,
	.flush		= cs1550_flush,
	.open		= cs1550_open,
	.init		= cs1550_init,
	.destroy	= cs1550_destroy,
};

/*
 * Don't change this.
 */
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &cs1550_oper, NULL);
}
