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

#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int hello_init(void)
{
	printk(KERN_ALERT "Hello Test0002\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye Test0002\n");
}

module_init(hello_init);
module_exit(hello_exit);

