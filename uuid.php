<?php
if(isset($_POST['key']))
{
  $key = trim($_POST['key']);
}

function uuid($chars)
{
  $uuid = array();
  
  $uuid[0] = substr($chars,0,8);
  $uuid[1] = substr($chars,8,4);
  $uuid[2] = substr($chars,12,4);
  $uuid[3] = substr($chars,16,4);
  $uuid[4] = substr($chars,20,12);
  $uuid[5] = substr($chars,32,8);
  
  return sprintf("%s-%s-%s-%s-%s-%s", $uuid[0], $uuid[1], $uuid[2], $uuid[3], $uuid[4], $uuid[5]);
}

function random_uuid()
{
  $chars = sha1(uniqid(mt_rand(), true));
  return uuid($chars);
}

function print_uuid()
{
  global $key;

  if(!empty($key))
  {
    echo "<color style=\"color:white;background-color:black\">".$key."</color>";
    echo PHP_EOL.PHP_EOL;
    echo uuid(sha1($key));
    return;
  }
  
  echo "Random UUIDs:\n";
  
  for($i = 0; $i < 16; $i++)
  {
    echo sprintf("%02d -> %s\n", $i + 1, random_uuid());
  }
}
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">

<html>

<head>
  <title>SHA1 UUID</title>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8">
  <meta http-equiv="expires" content="0">
</head>

<body>

<form action="uuid.php" method="post">
  <input type="input" name="key" size="48" value="">
  <input type="submit" value="Submit">
</form>

<br>

<pre>
<?php print_uuid(); ?>
</pre>

<br>

<form action="uuid.php" method="post">
  <input type="submit" value=" Refresh ">
</form>

</body>

</html>
