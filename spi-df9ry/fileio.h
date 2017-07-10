/* Copyright 2017 Tania Hagn
 *
 * This file is part of Daisy.
 *
 *    Daisy is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Daisy is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Daisy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/fs.h>

// Open a file:
extern struct file *file_open(const char *path, int flags, int rights);

// Close file:
extern void file_close(struct file *file);

// Read from file:
extern int file_read(struct file *file, unsigned long long offset,
		unsigned char *data, unsigned int size);

// Write to file:
extern int file_write(struct file *file, unsigned long long offset,
		unsigned char *data, unsigned int size);

// Assert that all data is written to the file:
extern int file_sync(struct file *file);
