<?php
require_once('worm_environment.php');

try{
  // Retrieve all non-simulated devices
  $query = "SELECT name FROM devices WHERE simulated_device = 0 ORDER BY name ASC";
  $sql->get_row($query,$devices);

  display_worm_page_header("Recent Scanner Activity");

  foreach($devices as $d){
    $device_name = $d[0];
    echo "<h3 id=\"d$device_name\">$device_name</h3>";

    // Get the four most recent captured images for this device
    $image_query = "SELECT c.image_id, c.small_image_id, c.capture_time "
      . "FROM captured_images AS c, capture_samples AS s "
      . "WHERE c.sample_id = s.id AND s.device_name = '$device_name' AND c.small_image_id != 0 "
      . "ORDER BY c.capture_time DESC LIMIT 4";
    $sql->get_row($image_query,$images);

    if (sizeof($images) == 0){
      echo "(No scans found)<br><br>";
      continue;
    }

    echo "<table cellspacing='5'><tr>";
    foreach ($images as $img){
      $full_id = $img[0];
      $small_id = $img[1];
      $time = format_time($img[2]);
      echo "<td><center><a href=\"ns_view_image.php?image_id=$full_id\">";
      echo "<img src=\"ns_view_image.php?image_id=$small_id&redirect=1\" alt=\"Scan\" width=\"200\"/></a><br>";
      echo "<font size='-1'>$time</font></center></td>";
    }
    echo "</tr></table><br>";
  }

  display_worm_page_footer();
}
catch(ns_exception $ex){
  die($ex->text);
}
?>
