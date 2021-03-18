
<?php
$servername = "localhost";
$username = "root";
$password = "peachpie";
$dbname = "mydb";
$date = date('Y-m-d H:i:s');
$temp = $_GET["temp"];
$press = $_GET["press"];
$p64 = $_GET["p64"];
$humid = $_GET["humid"];
// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
  die("Connection failed: " . $conn->connect_error);
}
$sql = "INSERT INTO `mydb`.`garden` (`date`, `temp`, `press`, `p64`, `humid`)
 VALUES ('" . $date."', ".$temp.", ".$press.", ".$p64.", ".$humid.")";
echo "query" . $sql;
if ($conn->query($sql) === TRUE) {
  echo "New record created successfully";
} else {
  echo "Error: " . $sql;
}

$conn->close();
?>
