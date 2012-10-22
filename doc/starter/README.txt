HAUS|PROX
---------
Be very careful when manually editing the card database file. (CARDS.TXT) 
The format is very strict and looks like this:

    FFF-CCCCC,E\n
    FFF-CCCCC,E\n
    FFF-CCCCC,E\n
    ...

Where FFF = the card facility code, left-zero padded if necessary 
            (exactly 3 digits)
      CCCCC = the card number, left-zero padded if necessary 
              (exactly 5 digits)
      E = enabled flag (0 or 1), whether the card will be accepted or not

Each line must end with a newline '\n' and there cannot be any trailing 
newlines in the file, or blank lines, or other lines that don't fit the 
format above. You can think of each line as a fixed length record of 
12 bytes.

NOTE: Some editors, especially on Windows machines, will add carriage return 
and newline characters to the ends of lines. Also be careful that your editor
doesn't add blank lines to the file. The hausprox program won't like this.

This means that the database file size must be a multiple of 12 bytes.

