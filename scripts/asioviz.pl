#!/usr/bin/perl -w
#
# asioviz.pl
# ~~~~~~~~~~
# A visualisation tool for asynchronous protocol implementations based on asio.
# Generates output intended for use with the GraphViz tool `dot'.
#
# Copyright (c) 2008-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

#-------------------------------------------------------------------------------
# Concepts
# ~~~~~~~~
# A visualisation is a directed graph comprised of nodes (representing
# functions or function objects) and edges (where a dashed edge represents an
# asynchronous call, and a solid edge is a synchronous call).
#
# Marking Up Source Code
# ~~~~~~~~~~~~~~~~~~~~~~
# Source code is marked up using special comments:
#
# - A function or function object is marked up with a comment of the form:
#
#     // @[nodename]
#
#   where nodename is a label for the node. All lines following the @[nodename]
#   comment are attached to the node, until all braces have been closed. This
#   typically means that the entire source of the function or function object
#   is included in the body of the node.
#
# - Asynchronous calls are marked with a comment of the form:
#
#     // @->[targetnodename]
#
#   and synchronous calls are marked with a comment of the form:
#
#     // @=>[targetnodename]
#
#   where targetnodename is a label for the target node. Target nodes will
#   always be shown in the visualisation even if they do not appear elsewhere in
#   the marked up source code.
#
# - Individual lines can be rendered in a specified colour using a comment of
#   the form:
#
#     // #ffffff
#
#   where `ffffff' can be replaced with any hexadecimal RGB triplet. Defaults to
#   black (#000000) if not specified.
#
# - If two markup comments are required on the same line, each needs its own
#   C++ comment:
#
#     // #ff0000 // @->[targetnodename]
#
# For example:
#
#   // @[handle_read]
#   // Handler for asynchronous reads.
#   void handle_read(const error_code& ec, size_t len)
#   {
#     if (!ec) // #ff0000
#     {
#       process_message(); // @=>[Process Message] // #00ff00
#       async_write(sock, buf, handle_write); // @->[handle_write]
#     }
#   }
#
#   // @[handle_write]
#   // Handle for asynchronous writes.
#   void handle_write(const error_code& ec, size_t len)
#   {
#     if (!ec)
#     {
#       async_read(sock, buf, handle_read); // @->[handle_read]
#     }
#   }
#
# Creating a Visualisation
# ~~~~~~~~~~~~~~~~~~~~~~~~
# The asioviz.pl script is used in conjunction with the GraphViz tool `dot' to
# generate images. To create a PNG file, use a pipeline of the form:
#
#   perl asioviz.pl < source.cpp | dot -Tpng > vis.png
#
# Or, to create a PDF, you might use a command similar to:
#
#   perl asioviz.pl < source.cpp | dot -Tpdf > vis.pdf
#
# Note that no default font name is specified. A specific font can be selected
# for the nodes using the `dot' command's `-N' parameter. For example:
#
#   perl asioviz.pl < source.cpp | dot -Tpdf -Nfontname=Consolas > vis.pdf
#-------------------------------------------------------------------------------

use strict;

my @nodes = ();
my @async_edges = ();
my @sync_edges = ();

#-------------------------------------------------------------------------------
# Parse the source file and populate the nodes, async_edges and sync_edges.

sub parse_source()
{
  my $current_node;
  my $brace_depth = 0;
  my $lineno = 0;
  while (my $line = <>)
  {
    ++$lineno;
    chomp($line);

    if (not $current_node and $line =~ /\@\[([^\]]*)\]/)
    {
      # Found the start of a new node.
      my %new_node = ( name=>$1 );
      $new_node{content} = ();
      $current_node = \%new_node;
      $brace_depth = 0;
    }
    elsif ($current_node)
    {
      # Check for a sync or async call.
      if ($line =~ /\/\/[ \t]*\@([-=])>\[([^\]]*)\]/)
      {
        $line =~ s/[ \t]*\/\/[ \t]*\@([-=])>\[([^\]]*)\]//;
        my $port = scalar(@{$current_node->{content}});
        my %edge = ( begin=>$current_node->{name}, port=>$port, end=>$2 );
        push(@async_edges, \%edge) if ($1 eq "-");
        push(@sync_edges, \%edge) if ($1 eq "=");
      }

      # Check for a custom font color.
      my $fontcolor = "#000000";
      if ($line =~ /\/\/[ \t]*(#[0-9a-fA-F]{6})/)
      {
        $line =~ s/[ \t]*\/\/[ \t]*(#[0-9a-fA-F]{6})//;
        $fontcolor = $1;
      }

      # Continuing an existing node.
      my %new_content = ( text=>$line, fontcolor=>$fontcolor, lineno=>$lineno );
      my $current_content = \%new_content;
      push(@{$current_node->{content}}, $current_content);
    }

    # Check for end of existing node.
    if ($current_node)
    {
      my $lbraces = $line;
      $lbraces =~ s/[^{]//g;
      $brace_depth += length($lbraces);

      my $rbraces = $line;
      $rbraces =~ s/[^}]//g;
      $brace_depth -= length($rbraces);

      if ($brace_depth <= 0 and length($rbraces) > 0)
      {
        # Found the end of the node.
        push(@nodes, $current_node);
        undef($current_node);
      }
    }
  }
}

#-------------------------------------------------------------------------------
# Helper function to convert a string to escaped HTML text.

sub escape($)
{
  my $text = shift;
  $text =~ s/&/\&amp\;/g;
  $text =~ s/</\&lt\;/g;
  $text =~ s/>/\&gt\;/g;
  $text =~ s/\t/    /g;
  return $text;
}

#-------------------------------------------------------------------------------
# Templates for dot output.

my $graph_header = <<"EOF";
/* Generated by asioviz.pl */
digraph G
{
graph [ nodesep="1", rankdir="TB" ];
node [ shape="box" ];
edge [ arrowtail="dot", tailclip="false", dir="both" ];
EOF

my $graph_footer = <<"EOF";
}
EOF

my $node_header = <<"EOF";
"%name%"
[
label=<<table border="0" cellspacing="0" cellpadding="1">
<tr><td colspan="3" align="center" bgcolor="gray" border="0">
<font point-size="11">%label%</font>
</td></tr>
EOF

my $node_footer = <<"EOF";
</table>>
]
EOF

my $odd_node_content = <<"EOF";
<tr><td align="right" bgcolor="white" border="0">
<font point-size="9" color="#a0a0a0">%lineno%: </font>
</td><td align="left" bgcolor="white" border="0">
<font point-size="9" color="%fontcolor%">%text%</font>
</td><td port="%port%" bgcolor="white" border="0"></td></tr>
EOF

my $even_node_content = <<"EOF";
<tr><td align="right" bgcolor="#f8f8f8" border="0">
<font point-size="9" color="#a0a0a0">%lineno%: </font>
</td><td align="left" bgcolor="#f8f8f8" border="0">
<font point-size="9" color="%fontcolor%">%text%</font>
</td><td port="%port%" bgcolor="#f8f8f8" border="0"></td></tr>
EOF

my $async_edges_header = <<"EOF";
{
edge [ style="dashed" arrowhead="open"];
EOF

my $async_edges_footer = <<"EOF";
}
EOF

my $sync_edges_header = <<"EOF";
{
edge [ style="solid" ];
EOF

my $sync_edges_footer = <<"EOF";
}
EOF

my $edge = <<"EOF";
"%begin%":%port%:c -> "%end%":n
EOF

#-------------------------------------------------------------------------------
# Generate dot output from the nodes, async_edges and sync_edges.

sub print_nodes()
{
  foreach my $node (@nodes)
  {
    my $name = $node->{name};
    my $label = escape($name);
    my $header = $node_header;
    $header =~ s/%name%/$name/g;
    $header =~ s/%label%/$label/g;
    print($header);

    my $port = 0;
    foreach my $content (@{$node->{content}})
    {
      my $text = escape($content->{text});
      my $fontcolor = $content->{fontcolor};
      my $lineno = $content->{lineno};
      my $line = ($port % 2 == 1) ? $even_node_content : $odd_node_content;
      $text = " " if length($text) == 0;
      $line =~ s/%text%/$text/g;
      $line =~ s/%port%/$port/g;
      $line =~ s/%fontcolor%/$fontcolor/g;
      $line =~ s/%lineno%/$lineno/g;
      print($line);
      ++$port;
    }

    print($node_footer);
  }
}

sub print_edges($$$)
{
  my $edges = shift;
  my $header = shift;
  my $footer = shift;

  print($header);
  foreach my $e (@{$edges})
  {
    my $begin = $e->{begin};
    my $port = $e->{port};
    my $end = $e->{end};
    my $line = $edge;
    $line =~ s/%begin%/$begin/g;
    $line =~ s/%port%/$port/g;
    $line =~ s/%end%/$end/g;
    print($line);
  }
  print($footer);
}

sub generate_dot()
{
  print($graph_header);
  print_nodes();
  print_edges(\@async_edges, $async_edges_header, $async_edges_footer);
  print_edges(\@sync_edges, $sync_edges_header, $sync_edges_footer);
  print($graph_footer);
}

#-------------------------------------------------------------------------------

parse_source();
generate_dot();