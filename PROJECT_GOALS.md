# Leaf Through Light — North Star Goal

## Project Name
**Leaf Through Light**

A real-time DirectX rendering demo following a drifting leaf through an endlessly streaming forest, built to showcase strong rendering fundamentals, scalable lighting, and performance-aware scene construction.

---

## Core Goal
Build a small but technically credible rendering demo that proves the following:

- I can design and structure a modern real-time rendering pipeline
- I can render a visually coherent scene with many dynamic lights
- I can use instancing and streaming to scale scene density efficiently
- I can think like an optimization-minded rendering engineer rather than only a gameplay engineer
- I can present technical work clearly, with debug tooling and measurable results

This project is not meant to be a full engine, a polished game, or an art piece for its own sake.
It is a focused rendering demo designed to communicate applied engineering strength.

---

## North Star Statement
Create a rendering demo that is **beautiful enough to hold attention, technical enough to earn respect, and lean enough to finish cleanly**.

---

## Project Pillars

### 1. Rendering Credibility
The demo should clearly demonstrate real rendering work rather than surface-level presentation.

This means the project should visibly center on:
- deferred rendering
- clustered lighting
- high instance counts
- efficient scene streaming
- frame-time awareness
- debugging and observability

### 2. Controlled Visual Identity
The scene should have a distinct mood and memorable framing.

The drifting leaf provides a simple narrative and camera motivation.
The forest provides density, repetition, light occlusion, and scale.
The visual identity should serve the renderer, not distract from it.

### 3. Performance as a First-Class Feature
Optimization is not cleanup at the end. It is part of the project’s identity.

The scene should be intentionally designed to create pressure on:
- light counts
- scene density
- draw submission
- streaming and chunk lifecycle
- frame-time stability

### 4. Expandable Without Scope Creep
The base demo should be complete on its own, but capable of supporting future additions.

Possible later extensions may include:
- post-processing improvements
- shadow improvements
- fog or volumetric effects
- more advanced material variation
- better procedural generation rules

These are optional expansions, not part of the core promise.

---

## What Success Looks Like
At the end of the project, the demo should allow someone to immediately understand:

- what the scene is
- what the main rendering techniques are
- why the chosen techniques make sense
- what performance problems were anticipated and addressed
- what technical tradeoffs were made

A successful result should feel like a compact rendering case study.

---

## Minimum Viable Outcome
The minimum acceptable version of the project includes:

- a functioning DirectX rendering demo
- a deferred rendering structure
- clustered lighting applied to a meaningful number of lights
- an endlessly streaming forest-style scene
- instanced repeated geometry
- a drifting leaf or equivalent forward scene anchor
- basic metrics and debug views
- a short written explanation of architecture and tradeoffs

If those pieces exist and are presented clearly, the project has succeeded.

---

## Non-Goals
To keep the project sharp, the following are explicitly not goals:

- building a full game
- building a general-purpose engine
- chasing maximum photorealism
- overbuilding editor or tooling systems
- implementing every modern graphics feature
- spending most of the time on asset production

Anything that does not strengthen the rendering case should be treated with suspicion.

---

## Design Principles

### Finish the spine first
The core rendering path, scene structure, and performance story matter more than secondary polish.

### Prefer evidence over claims
Every important technical decision should be something that can be shown through a debug mode, metric, comparison, or clear explanation.

### Use atmosphere in service of the renderer
The scene should be attractive, but the beauty should come from light, composition, density, and motion that validate the technical systems.

### Keep the demo legible
A reviewer should be able to understand the scene and its technical purpose quickly. Clean presentation matters.

### Keep the project honest
It is better to show one clean renderer with measurable reasoning than five half-finished features.

---

## Final Standard
If time becomes tight, the project should still end as:

**a clear, optimized, technically explainable rendering demo with a strong visual identity and real engineering intent.**
