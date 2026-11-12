# FaceTouchCorrection

`FaceTouchCorrection` provides reusable data types and editor UI for authoring manual face-touch targets with four-key additive correction timing.

The first editor integration is provided through `HandposeTimeline`:

1. Load an animation and use a selected actor.
2. Enable **FaceTouchCorrectionModus**.
3. Navigate to the intended touch frame.
4. Click the target actor's skeletal mesh in the level viewport.
5. Select a left- or right-hand interaction point.
6. Adjust the created correction block by dragging its edges, handles, or body.

Each block contains an approach zero, touch start, touch end, and recovery zero frame. Existing HandposeTimeline lane heights are unchanged.

The runtime module contains correction blocks, surface locations, and the optional correction data asset. The editor module contains viewport surface picking and the reusable correction lane. HandposeTimeline currently owns IK preview, correction deletion, and final animation baking.

## Modules

- `FaceTouchCorrection`: runtime data types and correction assets.
- `FaceTouchCorrectionEditor`: viewport picking and Slate authoring widgets.
