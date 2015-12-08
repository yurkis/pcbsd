<?php defined('DS') OR die('No direct access allowed.'); ?>
<div id="container" style="width:100%;height:100%;">

<nav id="nav" role="navigation">
	<a href="#nav" title="Show navigation">Show navigation</a>
	<a href="#" title="Hide navigation">Hide navigation</a>
	<ul class="clearfix">
		<li><a href="/">Home</a></li>
		<li>
			<a href="/?p=appcafe" aria-haspopup="true"><span><img src="/images/appcafe.png" height=24 width=24>AppCafe</span></a>
			<ul>
				<li><a href="/?p=sysapp"><img src="/images/install.png" height=24 width=24> Installed Applications</a></li>
				<li><a href="/?p=appcafe-browse"><img src="/images/categories.png" height=24 width=24> Browse Categories</a></li>
				<li><a href="/?p=appcafe-search"><img src="/images/search.png" height=32 width=32> App Search</a></li>
			</ul>
		</li>
		<li><a href="/?p=plugins"><img src="/images/jail.png" height=24 width=24>App Containers</a></li>
<?php
if (USERNAME)
echo "              <li><a href=\"/?logout=true\"><img src=\"/images/logout.png\" height=24 width=24> Logout</a></li>";
?>
	</ul>
</nav>

<body>

<script type="text/javascript">
$(document).ready(function () {
    var interval = 10000;   //number of mili seconds between each call
    var refresh = function() {
        $.ajax({
            url: "/pages/notifier.php",
            cache: false,
            success: function(html) {
                $('#notifier').html(html);
                setTimeout(function() {
                    refresh();
                }, interval);
            }
        });
    };
    refresh();
});
</script>
<div id="notifier"  onclick="location.href='/?p=dispatcher';" style="height:45px;width:350px;position:absolute;margin-top:0em;margin-left:55px;"></div>

<div id="body" style="height:100%;width=100%;float:left;margin-top:6.00em;margin-left:5px;margin-right:5px;">
