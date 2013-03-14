#!/usr/bin/env ruby
# Use OSX atos tool to symbolicate LLVM asan crashdump

ARGF.each { |line|
    match = /    #(\d+) (0x[0-9A-Fa-f]+) \((.+?)\+(0x[0-9A-Fa-f]+)\)/.match(line)
    frame, memadddr, exe, fileaddr = match.captures if match
    puts "    \##{frame} #{memadddr} #{`atos -arch x86_64 -o #{exe} #{fileaddr}`}" if match
    puts line unless match
}
