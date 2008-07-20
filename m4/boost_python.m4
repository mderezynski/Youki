<!DOCTYPE html
    PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">
<head>
 <title>#75: boost_python.m4 - MPX - Trac</title><link rel="start" href="/wiki" /><link rel="search" href="/search" /><link rel="help" href="/wiki/TracGuide" /><link rel="stylesheet" href="/chrome/common/css/trac.css" type="text/css" /><link rel="stylesheet" href="/chrome/common/css/code.css" type="text/css" /><link rel="icon" href="/chrome/site/favicon.ico" type="image/x-icon" /><link rel="shortcut icon" href="/chrome/site/favicon.ico" type="image/x-icon" /><link rel="up" href="/ticket/75" title="Ticket #75" /><link rel="alternate" href="/attachment/ticket/75/boost_python.m4?format=raw" title="Original Format" type="text/x-m4; charset=iso-8859-15" /><style type="text/css">
</style>
 <script type="text/javascript" src="/chrome/common/js/trac.js"></script>
</head>
<body>


<div id="banner">

<div id="header"><a id="logo" href="http://mpx.backtrace.info/"><img src="/chrome/site/backtrace_banner.png" alt="" /></a><hr /></div>

<form id="search" action="/search" method="get">
 <div>
  <label for="proj-search">Search:</label>
  <input type="text" id="proj-search" name="q" size="10" accesskey="f" value="" />
  <input type="submit" value="Search" />
  <input type="hidden" name="wiki" value="on" />
  <input type="hidden" name="changeset" value="on" />
  <input type="hidden" name="ticket" value="on" />
 </div>
</form>



<div id="metanav" class="nav"><ul><li class="first"><a href="/login">Login</a></li><li><a href="/settings">Settings</a></li><li><a accesskey="6" href="/wiki/TracGuide">Help/Guide</a></li><li class="last"><a href="/register">Register</a></li></ul></div>
</div>

<div id="mainnav" class="nav"><ul><li class="first"><a accesskey="1" href="/wiki">Wiki</a></li><li><a accesskey="2" href="/timeline">Timeline</a></li><li><a accesskey="3" href="/roadmap">Roadmap</a></li><li><a href="/browser">Browse Source</a></li><li><a href="/report">View Tickets</a></li><li class="last"><a href="/wiki/Downloads">Downloads</a></li></ul></div>
<div id="main">




<div id="ctxtnav" class="nav"></div>

<div id="content" class="attachment">


 <h1><a href="/ticket/75">Ticket #75</a>: boost_python.m4</h1>
 <table id="info" summary="Description"><tbody><tr>
   <th scope="col">
    File boost_python.m4, 3.5 kB 
    (added by grim,  2 minutes ago)
   </th></tr><tr>
   <td class="message"><p>
Updated boost_python.m4
</p>
</td>
  </tr>
 </tbody></table>
 <div id="preview">
   <table class="code"><thead><tr><th class="lineno">Line</th><th class="content">&nbsp;</th></tr></thead><tbody><tr><th id="L1"><a href="#L1">1</a></th>
<td># ===========================================================================</td>
</tr><tr><th id="L2"><a href="#L2">2</a></th>
<td>#&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; http://autoconf-archive.cryp.to/ax_boost_python.html</td>
</tr><tr><th id="L3"><a href="#L3">3</a></th>
<td># ===========================================================================</td>
</tr><tr><th id="L4"><a href="#L4">4</a></th>
<td>#</td>
</tr><tr><th id="L5"><a href="#L5">5</a></th>
<td># SYNOPSIS</td>
</tr><tr><th id="L6"><a href="#L6">6</a></th>
<td>#</td>
</tr><tr><th id="L7"><a href="#L7">7</a></th>
<td>#&nbsp; &nbsp;AX_BOOST_PYTHON</td>
</tr><tr><th id="L8"><a href="#L8">8</a></th>
<td>#</td>
</tr><tr><th id="L9"><a href="#L9">9</a></th>
<td># DESCRIPTION</td>
</tr><tr><th id="L10"><a href="#L10">10</a></th>
<td>#</td>
</tr><tr><th id="L11"><a href="#L11">11</a></th>
<td>#&nbsp; &nbsp;This macro checks to see if the Boost.Python library is installed. It</td>
</tr><tr><th id="L12"><a href="#L12">12</a></th>
<td>#&nbsp; &nbsp;also attempts to guess the currect library name using several attempts.</td>
</tr><tr><th id="L13"><a href="#L13">13</a></th>
<td>#&nbsp; &nbsp;It tries to build the library name using a user supplied name or suffix</td>
</tr><tr><th id="L14"><a href="#L14">14</a></th>
<td>#&nbsp; &nbsp;and then just the raw library.</td>
</tr><tr><th id="L15"><a href="#L15">15</a></th>
<td>#</td>
</tr><tr><th id="L16"><a href="#L16">16</a></th>
<td>#&nbsp; &nbsp;If the library is found, HAVE_BOOST_PYTHON is defined and</td>
</tr><tr><th id="L17"><a href="#L17">17</a></th>
<td>#&nbsp; &nbsp;BOOST_PYTHON_LIB is set to the name of the library.</td>
</tr><tr><th id="L18"><a href="#L18">18</a></th>
<td>#</td>
</tr><tr><th id="L19"><a href="#L19">19</a></th>
<td>#&nbsp; &nbsp;This macro calls AC_SUBST(BOOST_PYTHON_LIB).</td>
</tr><tr><th id="L20"><a href="#L20">20</a></th>
<td>#</td>
</tr><tr><th id="L21"><a href="#L21">21</a></th>
<td>#&nbsp; &nbsp;In order to ensure that the Python headers are specified on the include</td>
</tr><tr><th id="L22"><a href="#L22">22</a></th>
<td>#&nbsp; &nbsp;path, this macro requires AX_PYTHON to be called.</td>
</tr><tr><th id="L23"><a href="#L23">23</a></th>
<td>#</td>
</tr><tr><th id="L24"><a href="#L24">24</a></th>
<td># LAST MODIFICATION</td>
</tr><tr><th id="L25"><a href="#L25">25</a></th>
<td>#</td>
</tr><tr><th id="L26"><a href="#L26">26</a></th>
<td>#&nbsp; &nbsp;2008-04-12</td>
</tr><tr><th id="L27"><a href="#L27">27</a></th>
<td>#</td>
</tr><tr><th id="L28"><a href="#L28">28</a></th>
<td># COPYLEFT</td>
</tr><tr><th id="L29"><a href="#L29">29</a></th>
<td>#</td>
</tr><tr><th id="L30"><a href="#L30">30</a></th>
<td>#&nbsp; &nbsp;Copyright (c) 2008 Michael Tindal</td>
</tr><tr><th id="L31"><a href="#L31">31</a></th>
<td>#</td>
</tr><tr><th id="L32"><a href="#L32">32</a></th>
<td>#&nbsp; &nbsp;This program is free software; you can redistribute it and/or modify it</td>
</tr><tr><th id="L33"><a href="#L33">33</a></th>
<td>#&nbsp; &nbsp;under the terms of the GNU General Public License as published by the</td>
</tr><tr><th id="L34"><a href="#L34">34</a></th>
<td>#&nbsp; &nbsp;Free Software Foundation; either version 2 of the License, or (at your</td>
</tr><tr><th id="L35"><a href="#L35">35</a></th>
<td>#&nbsp; &nbsp;option) any later version.</td>
</tr><tr><th id="L36"><a href="#L36">36</a></th>
<td>#</td>
</tr><tr><th id="L37"><a href="#L37">37</a></th>
<td>#&nbsp; &nbsp;This program is distributed in the hope that it will be useful, but</td>
</tr><tr><th id="L38"><a href="#L38">38</a></th>
<td>#&nbsp; &nbsp;WITHOUT ANY WARRANTY; without even the implied warranty of</td>
</tr><tr><th id="L39"><a href="#L39">39</a></th>
<td>#&nbsp; &nbsp;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General</td>
</tr><tr><th id="L40"><a href="#L40">40</a></th>
<td>#&nbsp; &nbsp;Public License for more details.</td>
</tr><tr><th id="L41"><a href="#L41">41</a></th>
<td>#</td>
</tr><tr><th id="L42"><a href="#L42">42</a></th>
<td>#&nbsp; &nbsp;You should have received a copy of the GNU General Public License along</td>
</tr><tr><th id="L43"><a href="#L43">43</a></th>
<td>#&nbsp; &nbsp;with this program. If not, see &lt;http://www.gnu.org/licenses/&gt;.</td>
</tr><tr><th id="L44"><a href="#L44">44</a></th>
<td>#</td>
</tr><tr><th id="L45"><a href="#L45">45</a></th>
<td>#&nbsp; &nbsp;As a special exception, the respective Autoconf Macro's copyright owner</td>
</tr><tr><th id="L46"><a href="#L46">46</a></th>
<td>#&nbsp; &nbsp;gives unlimited permission to copy, distribute and modify the configure</td>
</tr><tr><th id="L47"><a href="#L47">47</a></th>
<td>#&nbsp; &nbsp;scripts that are the output of Autoconf when processing the Macro. You</td>
</tr><tr><th id="L48"><a href="#L48">48</a></th>
<td>#&nbsp; &nbsp;need not follow the terms of the GNU General Public License when using</td>
</tr><tr><th id="L49"><a href="#L49">49</a></th>
<td>#&nbsp; &nbsp;or distributing such scripts, even though portions of the text of the</td>
</tr><tr><th id="L50"><a href="#L50">50</a></th>
<td>#&nbsp; &nbsp;Macro appear in them. The GNU General Public License (GPL) does govern</td>
</tr><tr><th id="L51"><a href="#L51">51</a></th>
<td>#&nbsp; &nbsp;all other use of the material that constitutes the Autoconf Macro.</td>
</tr><tr><th id="L52"><a href="#L52">52</a></th>
<td>#</td>
</tr><tr><th id="L53"><a href="#L53">53</a></th>
<td>#&nbsp; &nbsp;This special exception to the GPL applies to versions of the Autoconf</td>
</tr><tr><th id="L54"><a href="#L54">54</a></th>
<td>#&nbsp; &nbsp;Macro released by the Autoconf Macro Archive. When you make and</td>
</tr><tr><th id="L55"><a href="#L55">55</a></th>
<td>#&nbsp; &nbsp;distribute a modified version of the Autoconf Macro, you may extend this</td>
</tr><tr><th id="L56"><a href="#L56">56</a></th>
<td>#&nbsp; &nbsp;special exception to the GPL to apply to your modified version as well.</td>
</tr><tr><th id="L57"><a href="#L57">57</a></th>
<td></td>
</tr><tr><th id="L58"><a href="#L58">58</a></th>
<td>AC_DEFUN([AX_BOOST_PYTHON],</td>
</tr><tr><th id="L59"><a href="#L59">59</a></th>
<td>[AC_REQUIRE([AX_PYTHON])dnl</td>
</tr><tr><th id="L60"><a href="#L60">60</a></th>
<td>AC_CACHE_CHECK(whether the Boost::Python library is available,</td>
</tr><tr><th id="L61"><a href="#L61">61</a></th>
<td>ac_cv_boost_python,</td>
</tr><tr><th id="L62"><a href="#L62">62</a></th>
<td>[AC_LANG_SAVE</td>
</tr><tr><th id="L63"><a href="#L63">63</a></th>
<td>&nbsp;AC_LANG_CPLUSPLUS</td>
</tr><tr><th id="L64"><a href="#L64">64</a></th>
<td>&nbsp;CPPFLAGS_SAVE=$CPPFLAGS</td>
</tr><tr><th id="L65"><a href="#L65">65</a></th>
<td>&nbsp;if test x$PYTHON_INCLUDE_DIR != x; then</td>
</tr><tr><th id="L66"><a href="#L66">66</a></th>
<td>&nbsp; &nbsp;CPPFLAGS=-I$PYTHON_INCLUDE_DIR $CPPFLAGS</td>
</tr><tr><th id="L67"><a href="#L67">67</a></th>
<td>&nbsp;fi</td>
</tr><tr><th id="L68"><a href="#L68">68</a></th>
<td>&nbsp;AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[</td>
</tr><tr><th id="L69"><a href="#L69">69</a></th>
<td>&nbsp;#include &lt;boost/python/module.hpp&gt;</td>
</tr><tr><th id="L70"><a href="#L70">70</a></th>
<td>&nbsp;using namespace boost::python;</td>
</tr><tr><th id="L71"><a href="#L71">71</a></th>
<td>&nbsp;BOOST_PYTHON_MODULE(test) { throw &#34;Boost::Python test.&#34;; }]],</td>
</tr><tr><th id="L72"><a href="#L72">72</a></th>
<td>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;[[return 0;]]),</td>
</tr><tr><th id="L73"><a href="#L73">73</a></th>
<td>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;ac_cv_boost_python=yes, ac_cv_boost_python=no)</td>
</tr><tr><th id="L74"><a href="#L74">74</a></th>
<td>&nbsp;AC_LANG_RESTORE</td>
</tr><tr><th id="L75"><a href="#L75">75</a></th>
<td>&nbsp;CPPFLAGS=$CPPFLAGS_SAVE</td>
</tr><tr><th id="L76"><a href="#L76">76</a></th>
<td>])</td>
</tr><tr><th id="L77"><a href="#L77">77</a></th>
<td>if test &#34;$ac_cv_boost_python&#34; = &#34;yes&#34;; then</td>
</tr><tr><th id="L78"><a href="#L78">78</a></th>
<td>&nbsp; AC_DEFINE(HAVE_BOOST_PYTHON,,[define if the Boost::Python library is available])</td>
</tr><tr><th id="L79"><a href="#L79">79</a></th>
<td>&nbsp; ax_python_lib=boost_python</td>
</tr><tr><th id="L80"><a href="#L80">80</a></th>
<td>&nbsp; AC_ARG_WITH([boost-python],AS_HELP_STRING([--with-boost-python],[specify the boost python library or suffix to use]),</td>
</tr><tr><th id="L81"><a href="#L81">81</a></th>
<td>&nbsp; [if test &#34;x$with_boost_python&#34; != &#34;xno&#34;; then</td>
</tr><tr><th id="L82"><a href="#L82">82</a></th>
<td>&nbsp; &nbsp; &nbsp;ax_python_lib=$with_boost_python</td>
</tr><tr><th id="L83"><a href="#L83">83</a></th>
<td>&nbsp; &nbsp; &nbsp;ax_boost_python_lib=boost_python-$with_boost_python</td>
</tr><tr><th id="L84"><a href="#L84">84</a></th>
<td>&nbsp; &nbsp;fi])</td>
</tr><tr><th id="L85"><a href="#L85">85</a></th>
<td>&nbsp; for ax_lib in $ax_python_lib $ax_boost_python_lib boost_python; do</td>
</tr><tr><th id="L86"><a href="#L86">86</a></th>
<td>&nbsp; &nbsp; AC_CHECK_LIB($ax_lib, exit, [BOOST_PYTHON_LIB=-l$ax_lib break])</td>
</tr><tr><th id="L87"><a href="#L87">87</a></th>
<td>&nbsp; done</td>
</tr><tr><th id="L88"><a href="#L88">88</a></th>
<td>&nbsp; AC_SUBST(BOOST_PYTHON_LIB)</td>
</tr><tr><th id="L89"><a href="#L89">89</a></th>
<td>fi</td>
</tr><tr><th id="L90"><a href="#L90">90</a></th>
<td>])dnl</td>
</tr></tbody></table>
 </div>
 


</div>
<script type="text/javascript">searchHighlight()</script>
<div id="altlinks"><h3>Download in other formats:</h3><ul><li class="first last"><a href="/attachment/ticket/75/boost_python.m4?format=raw">Original Format</a></li></ul></div>

</div>

<div id="footer">
 <hr />
 <a id="tracpowered" href="http://trac.edgewall.org/"><img src="/chrome/common/trac_logo_mini.png" height="30" width="107"
   alt="Trac Powered"/></a>
 <p class="left">
  Powered by <a href="/about"><strong>Trac 0.10.4</strong></a><br />
  By <a href="http://www.edgewall.org/">Edgewall Software</a>.
 </p>
 <p class="right">
  MPX, media player for X
 </p>
</div>



 </body>
</html>

