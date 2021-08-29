# Phrase-locked MIDI plugins for live performance

Simple MIDI plugins which apply your parameters relevant to phrase boundaries.

- A _phrase_ is defined as a number of beats - e.g. 1 bar / 4 beats, up to 64 beats.
- Each plugin has a parameter for phrase length.
- As the DAW transport plays back, the plugin keeps track of where it is in the current phrase.

## MIDI clip variations
Phrase-synchable MIDI filter plugins for getting more out of MIDI clips for live performance.

- `ClipVariations-Note.vst3` Uses note value ranges for each variation, e.g. C-1:C0 for variation 1 and so on. Notes are transposed so each variation is consistent pitch.
- `ClipVariations-Channel.vst3` Uses channels. Each MIDI channel is a separate variation. 

Each plugin has a variation parameter. You select a variation, and on the next phrase boundary, it will take effect.

In your DAW, for example Ableton Live or Bitwig, duplicate notes in your midi clip and transpose up (notes) or change channel (channel). Now you have variations that are the same notes.

Edit the notes for each variation, for example:

- Add different drum fills or ad-libs to a beat clip.
- Increase or decrease the density of notes in a pad or arpeggio clip. 

Insert the appropriate plugin in the track, and use the channel/variation parameter to switch.

The variation will only switch on phrase boundaries, so changes happen in sync. For example, you could switch from a "drop" beat pattern to a "break", or chorus to verse. There's another parameter for phrase length (4 - 64 beats).

## Controller motion
- `ControllerMotion.vst3` allows you to animate 4 MIDI CC values towards a target value. 

Set a phrase length. Then adjust the target parameters to the desired target value. 

The plugin will output MIDI CCs that approach the target value at the end of the phrase.

For example, you could map MIDI CC 1 to cutoff frequency for an arpeggiator pattern, and set phrase to 32 beats. Any changes you make to target value 1 will unfold over <32 beats, and always hit the target on the next phrase boundary.

If you want to disable the animation, you can set phrase length to 1 beat for a very quick ramp.

## How to dev
This project is built using [JUCE](https://juce.com). 

- Clone this repository.
- Open project file in the `Projucer` and export a build project (e.g. for `Xcode` on macOS).

From there you can build and debug as normal.
