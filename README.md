# foo_dsp_vlevel

This project implements VLevel for the foobar2000 media player.

VLevel is a dynamic range compressor, which can help amplify the quiet parts of
music. It was developed by Tom Felker at <http://vlevel.sf.net/>.

This repository provides more up-to-date builds (including 64-bit support) for
the VLevel plugin.

## Building

1. Download the foobar2000 SDK from <https://www.foobar2000.org/SDK>.
2. Copy the contents of the downloaded archive to this repository.

   The location of `foobar2000/foo_dsp_vlevel` should replace
   `foobar2000/foo_sample` provided by the SDK. Keep other parts of the
   SDK's directory similar to the project.

   Basically, the resulting project should look something like:

   ```txt
   # Project files
   /.editorconfig
   /.gitignore
   /foobar2000/foo_dsp_vlevel/

   # SDK files
   /sdk-license.txt
   /sdk-readme.html
   /libPPUI/
   /pfc/
   /foobar2000/foo_input_validator/
   /foobar2000/foobar2000_component_client/
   /foobar2000/helpers/
   /foobar2000/SDK/
   /foobar2000/shared/
   ```

3. Open the solution file at `foobar2000/foo_dsp_vlevel/foo_dsp_vlevel.sln` with
   Visual Studio.
4. Add the WTL library to these projects (via NuGet package manager):
   * `foo_dsp_vlevel` (should already be added)
   * `foobar2000_sdk_helper`
   * `libPPUI`
5. Build the solution.

Note: The current archive is created manually for now so there's no script. Just
create a zip archive with the following structure:

```txt
foo_dsp_vlevel.dll     - 32-bit DLL
x64/foo_dsp_vlevel.dll - 64-bit DLL
```

and rename the extension from `.zip` to `.fb2k-component`.

## Authors

Seally (current)
Tom Felker
wore <info@wore.ma.cx>
Wiesl

## Changelog

Refer to Git logs for current changelogs.

### Old Changelog

* Sun Mar 2 2008 Wiesl 20080302.0
  * Updated README
  * Updated Version

* Wed Nov 7 2007 Wiesl 20071107.0
  * made ready for 0.9.x series
  * config settings now work well, added useful limits for max multiplier
  * added dB scale
  * added debug code by wiesl

* Tue Jun 1 2004 wore <info@wore.ma.cx> 20040601.5
  * some fix to compile.

* Mon May 31 2004 wore <info@wore.ma.cx> 20040530.4-tom
  * I got great mail with new code from Tom, the VLevel author.
  He made many modifications to make it stable.
  * I'll put binary later.

* Sun May 30 2004 wore <info@wore.ma.cx> 20040530.3
  * I've fprintf debug to find why Access violation occured.
    Normally, on_chunk is called with 1024 samples.
    But, when fb2k crash, on_chunk is called with more than 88200 samples.
    Buffers allocated only 44100 or 48000 samples.
  * To fix this bug, I allocate buffer for 5 seconds.
    But it is not good way.
    I'd like to rewrite on_chunk till this weekend.

* Sun May 30 2004 wore <info@wore.ma.cx> 20040530.2
  * remove vl=0; in cleanup_buffers and tested it, but still crashes.

* Sun May 30 2004 wore <info@wore.ma.cx> 20040530
  * initial version.
  * based on vlevel-bin.cpp and foobar2k SDK's hardlimit.cpp
