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

#ifndef _RFM22B_H_
#define _RFM22B_H_

struct rfm22b;

extern int rfm22b_reg_write(struct rfm22b *rfm22b, u8 reg, u8 data);
extern int rfm22b_reg_read(struct rfm22b *rfm22b, u8 reg);
extern int rfm22b_block_read(struct rfm22b *rfm22b, u8 reg, u8 length,
			    u8 *values);
extern int rfm22b_block_write(struct rfm22b *rfm22b, u8 reg, u8 length,
			     const u8 *values);
extern int rfm22b_set_bits(struct rfm22b *rfm22b, u8 reg, u8 mask, u8 val);
extern int rfm22b_enable(struct rfm22b *rfm22b, unsigned int blocks);
extern int rfm22b_disable(struct rfm22b *rfm22b, unsigned int blocks);

#endif /* _RFM22B_H_ */
