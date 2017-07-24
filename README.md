# Audio Tracker

This repository is really three parts.  

1. The audio plugin built with JUCE.
2. The web server that accepts post request made by the audio plugin.
3. The web frontend that queries the server and graphs the results.

The idea roughly is you can monitor audio metrics of a track that is being recorded remotely.

# Installation

`brew install golang`  
`brew install yarn`

There may be some deps i'm missing :)

You will likely have to run projucer and click "Save and open in IDE".  Xcode will open - then run the build.  After the build succeeds you can open your DAW.

**Note: Only AU is built but you can turn the other targets on in projcuer.

Run the server with `go run main.go`

Run the frontend by moving into the app folder and executing `yarn start`

# Usage

1. Add the plugin to the audio channel you want to measure.
2. Hit play
3. When the `silenceHeard` threshold is met, the mean measurement for all props will be sent to the server.  

# Props

I made use of MANY other great repos.  They are listed here:

1. JUCE - https://www.juce.com/
2. https://github.com/adamski/RestRequest
3. Gist - https://github.com/adamstark/Gist