# MIDI Clip Phrase Variations

Phrase-synchable MIDI filter plugins for getting more out of MIDI clips for live performance.

- `ClipVariations-Note.vst3` Uses note value ranges for each variation, e.g. C-1:C0 for variation 1 and so on. Notes are transposed so each variation is consistent pitch.
- `ClipVariations-Channel.vst3` Uses channels. Each MIDI channel is a separate variation. 

## How to use
In your DAW, for example Ableton Live or Bitwig, duplicate notes in your midi clip and transpose up (notes) or change channel (channel). Now you have variations that are the same notes.

Edit the notes for each variation, for example:

- Add different drum fills or ad-libs to a beat clip.
- Increase or decrease the density of notes in a pad or arpeggio clip. 

Insert the appropriate plugin in the track, and use the channel/variation parameter to switch.

The variation will only switch on phrase boundaries, so changes happen in sync. For example, you could switch from a "drop" beat pattern to a "break", or chorus to verse. There's another parameter for phrase length (4 - 64 beats).

## How to dev
This project is built using [JUCE](https://juce.com). 

- Clone this repository.
- Open project file in the `Projucer` and export a build project (e.g. for `Xcode` on macOS).

From there you can build and debug as normal.
