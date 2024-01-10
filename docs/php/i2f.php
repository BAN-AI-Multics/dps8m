#!/usr/bin/env php
<?php
/*
 * vim: filetype=php:tabstop=2:ai:tw=78:expandtab
 * SPDX-License-Identifier: MIT
 * scspell-id: c647fe48-fc68-11ec-9e3d-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * i2f.php - Convert Markdown indented-style to fenced-style code blocks.
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Tobias Potocek <tobiaspotocek@gmail.com>
 * Copyright (c) 2021-2024 The DPS8M Development Team
 *
 * ---------------------------------------------------------------------------
 */

$filename = $argv[1];
$content = file_get_contents($filename);

function isCode($line)
  {
    return preg_match("/^    /", $line);
  }

function removeIndent($line)
  {
    return preg_replace("/^    /", "", $line);
  }

function isEmpty($line)
  {
    return $line == "" || preg_match("/^ *$/", $line);
  }

$inCode = false;
$input = explode("\n", $content);
$output = [];

foreach ($input as $line)
  {
	if ($inCode)
	  {
		if (isEmpty($line))
		  {
            $output[] = $line;
		  }
		    else if (isCode($line))
		  {
            $output[] = removeIndent($line);
		  }
			else
		  {
            $output[count($output) - 1] = "```";
            $output[] = "";
            $output[] = $line;
            $inCode = false;
          }
      }
	    else
	  {
		if (isCode($line))
		  {
            $output[] = "```dps8";
            $output[] = removeIndent($line);
            $inCode = true;
		  }
		    else
		  {
            $output[] = $line;
          }
      }
  }

file_put_contents($filename, join("\n", $output));
?>
