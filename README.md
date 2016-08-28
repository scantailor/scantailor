# Scan Tailor - [scantailor.org](http://scantailor.org/)

![ScanTailor logo from scantailor.org](http://scantailor.org/assets/logo_h300-fs8.png) 


## About ##

Scan Tailor is an interactive post-processing tool for scanned pages. 
It performs operations such as:
  - [page splitting](https://github.com/scantailor/scantailor/wiki/Split-Pages), 
  - [deskewing](https://github.com/scantailor/scantailor/wiki/Deskew), 
  - [adding/removing borders](https://github.com/scantailor/scantailor/wiki/Page-Layout), 
  - [selecting content](https://github.com/scantailor/scantailor/wiki/Select-Content) 
  - ... and others. 
  
You give it raw scans, and you get pages ready to be printed or assembled into a PDF 
  or [DJVU](http://elpa.gnu.org/packages/djvu.html) file. Scanning, optical character recognition, 
  and assembling multi-page documents are out of scope of this project.

Scan Tailor is [Free Software](https://www.gnu.org/philosophy/free-sw.html) (which is more than just freeware). 
  It’s written in C++ with Qt and released under the General Public License version 3. 
  We develop both Windows and GNU/Linux versions.

## History and Future

This project started in late 2007 and by mid 2010 it reached production quality. 

In 2014, the original developer [Joseph Artsimovich](https://github.com/Tulon) stepped aside, 
and [Nate Craun](https://natecraun.net/) ([@ncraun](https://github.com/ncraun)) 
  took over as the [new maintainer](http://scantailor.org/2014/04/06/new-maintainer.html).

For information on contributing and the longstanding plan for the project, please see the 
  [Roadmap](https://github.com/scantailor/scantailor/wiki/Roadmap-1.0) wiki entry.
  
For any suggested changes or bugs, please consult the [Issues](https://github.com/scantailor/scantailor/issues) tab.

## Usage

Scan Tailor is being used not just by enthusiasts, but also by libraries and other institutions. 
  Scan Tailor processed books can be found on Google Books and the Internet Archive. 
  - [Prolog for Programmers](https://sites.google.com/site/prologforprogrammers/the-book). The 47.3MB pdf is the original, 
    and the 3.1MB pdf is after using Scan Tailor. The OCR, Chapter Indexing, JBIG2 compression, and PDF Binding were not 
    done with Scan Tailor, but all of the scanned image cleanup was. [[1](scantailor.org/downloads/)]
  - [Oakland Township: Two Hundred Years](http://books.google.com/books?printsec=frontcover&id=o4Q2OlVl61MC) 
      by Stuart A. Rammage (also available: volumes 2, 3, 4.1, 4.2, 5.1, and 5.2) [[2](http://www.diybookscanner.org/forum/viewtopic.php?t=435)]
  - [Herons and Cobblestones](http://books.google.com/books?printsec=frontcover&id=o4Q2OlVl61MC): A History of Bethel and the Five Oaks Area of Brantford Township, 
      County of Brant by the Grand River Heritage Mines Society [[2](http://www.diybookscanner.org/forum/viewtopic.php?t=435)]


## Installation and Tips
  
[Scanning Tips](https://github.com/scantailor/scantailor/wiki/Tips-for-Scanning), 
  [Quick-Start-Guide](https://github.com/scantailor/scantailor/wiki/Quick-Start-Guide), and complete 
  [Usage Guide](https://github.com/scantailor/scantailor/wiki/User-Guide), including installation information 
  (via the [installer](https://github.com/scantailor/scantailor/wiki/User-Guide#installation-and-first-start) or 
  [building from from source](https://github.com/scantailor/scantailor/wiki/Building-from-Source-Code-on-Linux-and-Mac-OS-X))
  can be found in the [wiki](https://github.com/scantailor/scantailor/wiki/)!

## Additional Links 

- [Linux Guide to Book Scanning](https://natecraun.net/articles/linux-guide-to-book-scanning.html)
- [DIY Book Scanner Forum](http://diybookscanner.org/forum/)
- [Scan Tailor Subforum](http://diybookscanner.org/forum/viewforum.php?f=21) from the above.
