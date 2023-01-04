#!/usr/bin/env php
<?php
/*
 * vim: filetype=php:tabstop=2:ai:tw=78:expandtab
 * SPDX-License-Identifier: ICU
 * scspell-id: c647fe48-fc68-11ec-9e3d-80ee73e9b8e7
 *
 * ---------------------------------------------------------------------------
 *
 * i2f.php - Convert Markdown indented-style to fenced-style code blocks.
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2016 Tobias Potocek <tobiaspotocek@gmail.com>
 * Copyright (c) 2021-2023 The DPS8M Development Team
 *
 * All rights reserved.
 *
 * This software is made available under the terms of the ICU
 * License, version 1.8.1 or later.  For more details, see the
 * LICENSE.md file at the top-level directory of this distribution.
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
