# mcml -- Multi-Column Markup Language

## A tiny DSL for displaying columnar data

`mcml` takes in arbitrary data and formats it in columns
much the way the GNU/Linux `ls` command lists in a multi-column
format (except for long format listings).

In fact, the heart of the logic for determining how many columns
will fit is taken right out of the GNU coreutils `ls` command.

But, `mcml` knows nothing of file names.
Input is one element per line.
Each element can have any characters, except, of course,
newline and the nul byte.

The logic is useful for things like listing package names,
command names, user names, etc.

Optionally, commands can be mixed in with data.
The --cmd option specifies a command prefix.

The only command is, "category",
which introduces a new category.
For every new category, `mcml` prints what elements
have been accumulated so far (for the previous category),
then prints the new category header,
then starts accumulating elements for the next category.

Once a category has been printed,
it does not influence the way columns are laid out
for the next category.

## Options and commands

--debug

Pretty-print values of interest only for debugging.

--verbose

Show some feedback while running.

--cmd=_str_

Use _str_ as the command prefix.


--horizontal

Arrange elements horizontally, across each row.

So, { alice bob charly david } is laid out like

```
{ alice   bob   }

{ charly  david }
```

The default is to print colmns,
like newspaper columns.
So, { alice bob charly david } is laid out like

```
{ alice  charly }
{ bob    david  }
```


--width=_n_

Use _n_ as the dsplay width.

The default is 80 characters.
`mcml` knows nothing of fonts,
and certainly does not provide for variable width fonts.

`mcml` does not try to determine display width
using any knowledge of terminals or any environment variables
such as `COLUMNS`.  The caller will have to take care of that.


--indent=_n_

Indent each data line with _n_ spaces.
No provision is made for tabs.
Category headers get printed with no indentation.


## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

##

-- Guy Shaw

   gshaw@acm.org

