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
 
import 'package:angular2/platform/browser.dart';
import 'package:angular2/angular2.dart';

import 'package:daisy_gui/daisy_gui.dart';

@Input() int    smeter = 0;
@Input() String title  = "Oeha";
    
void main() {
smeter = -90;
  bootstrap(AppComponent);
  title = "Blub";
}
