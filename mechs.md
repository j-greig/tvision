# Mechanoids
A generative art series, Mechanoids (mechs) combine monospaced ASCII art with emoji faces/organs/extras.

## Mechanoid Structure
Mechs are drawn using box drawing characters [reference unicode block here]. Emoji can be used to portray their faces, organs, or environment.

### Composition
Canvas: 19w x 9h

### Rules
- If a mech has feet, they must be anchored to the ground, eg touching the last row
- Mechs must have: head with at least once face component (eyes or mouth), body
- Legs and arms are optional
- Legs and arms must join the body
- Leg and arm lengths should be adjusted to ensure the mech falls within the canvas bounds

### Allowed Emoji
Only whitelisted emoji can be used:
```js
const ALLOWED_EMOJI = {
  eyes: ['👁️', '👁', '💀', '💿', '◌', '◡'],
  mouth: ['🫦'],
  organs: ['🫁', '🧠', '🫀'],
  planets: ['🌏', '🌎', '🌍'],
  elements: ['🌊'],
  creatures: ['👹'],
  patterns: ['▚', '◲', '◱', '◰', '◳', '✜']
};
```

### Border Style System

**Specification Format**: All Mechanoids defined using single-line box drawing characters:
```
Standard chars: ─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼
```

**Runtime Styles**: Any `drawbox.js` border style can be applied during generation:
```js
// Available from src/modules/drawbox.js
const BORDER_STYLES = ['single', 'double', 'round', 'fat', 'singleDouble', 'none'];

// Style substitution - spec chars map to runtime style chars
function applyBorderStyle(specChar, styleName) {
  const style = borderStyles[styleName] || borderStyles.single;
  const charMap = {
    '─': style.top,     '│': style.left,
    '┌': style.topleft, '┐': style.topright, 
    '└': style.bottomleft, '┘': style.bottomright,
    '├': style.left,    '┤': style.right,
    '┬': style.top,     '┴': style.bottom,
    '┼': style.topleft  // junctions use corner chars
  };
  return charMap[specChar] || specChar;
}
```

**Usage**: 
- Spec: `┌───┐` (single-line standard)
- Render: `╭───╮` (round), `╔═══╗` (double), `█▀▀▀█` (fat), etc.

### Implementation Example

```js
// Convert spec mechanoid to styled version
function styleMechanoid(specLines, borderStyle = 'round') {
  return specLines.map(line => {
    return line.split('').map(char => {
      // Skip emoji and spaces
      if (char === ' ' || /[\u{1F600}-\u{1F64F}]/u.test(char)) return char;
      return applyBorderStyle(char, borderStyle);
    }).join('');
  });
}

// Example usage
const spec = [
  "┌──┬──┬──┐",
  "│👁️│🫦│👁️│", 
  "└──┴──┴──┘"
];

const roundMech = styleMechanoid(spec, 'round');
// Result: ["╭──┬──┬──╮", "│👁️│🫦│👁️│", "╰──┴──┴──╯"]

const doubleMech = styleMechanoid(spec, 'double'); 
// Result: ["╔══╦══╦══╗", "║👁️║🫦║👁️║", "╚══╩══╩══╝"]
```

### Body Parts
#### Heads

##### Faces
**Face Wide**
```
╭──┬──┬──╮
│👁️│  ︪│👁️️│
╰──┴╮╭┴──╯
```
---
** Face Normal**
```
╭──────╮
┤  👁  ├
╰──────╯
```
---
** Face Skinny**
```
┌──┐
│👹️│
└──┘
```
---

#### Body
**Out**
```╭───```
```───╮```

#### Arms

#### Legs
** Leg Normal Right-Angled
```
╭───
│   
│   
```
```
───╮
   │
   │
```
** Leg Narrow Right-Angled
```
┌─
│ 
│  
```
```
┌─
│ 
│ 
```

#### Feet
**Tripod Large**
```
 ╱│╲ 
╱ │ ╲
```
**Tripod Large Cutoff**
```
 ╱│╲ 
╱   ╲
```
**Tripod Small**
```
╱│╲
```
**Prong**
```
╔╬╗
```
**Pring Wide**
```
┌─┼─┐
```
**Clank**
```
┌───┐
```
**Skinny**
```
│
```
... and any other parts or systems you can derive from the systems below.

## Mechanoid Examples
    ┌──┬──┬──┐    
    │👁️│🫦│👁️️│    
    ╰──┴╮╭┴──╯    
      ╭─╯╰─╮      
  ┌───┤▚▚▚▚├───┐  
  │   ╰────╯   │  
  │            │  
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲
---
    ╭╵╵╵╵╵╵╵╵╮    
    │   🌏   ️️│    
    ╰───╮╭───╯    
      ╭─╯╰─╮      
  ┌───┤    ├───┐  
  │   ╰────╯   │  
  │            │  
  │            │  
┌───┐        ┌───┐
---
    ╭──┬──┬──╮    
    │👁️│  ︪│👁️️│   
    ╰──┴╮╭┴──╯    
      ╭─╯╰─╮      
  ┌───┤◲◱◰◳├───┐  
  │   ╰────╯   │  
  │            │  
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲
---
     ╭──────╮    
     ┤  👁  ├    
     ╰──────╯   
    ╭────────╮    
  ┌─┤ 🫁🫁🫁 ├─┐  
  │ ╰────────╯ │  
  │            │  
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲
---
    ╔══╬══╬══╗    
    ╬👁️╬🫦╬👁️╬    
    ╚══╬══╬══╝    
      ╔════╗      
  ╔═╬═╣╬╬╬╬╠═╬═╗  
  ╬   ╚════╝   ╬  
  ╬            ╬  
  ╬            ╬  
 ╔╬╗          ╔╬╗  
 ---
    ╭──┬──┬──╮    
    │👁️│  │👁️️│    
    ╰──┴╮╭┴──╯    
      ╭─╯╰─╮      
  ╭───┤✜✜├───╮  
      ╰────╯       
       │  │  
       │  │  
       │  │  
---
    ╭────────╮    
    │ 👁  👁️ │    
    ╰────────╯   
    ╭────────╮    
  ┌─┤ 🌎🌍🌏 ├─┐  
  │ ╰────────╯ │  
  │            │  
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲
---
       ┌──┐
  ┌────│👹️│────┐
  │    └──┘    │
  │            │
  │            │
  │            │
  │            │
  │            │
 ╱│╲          ╱│╲
 ---
    ╭──┬──┬──╮  
    │👁️│  │👁️️│  
    ╰──┴╮╭┴──╯
      ╭─╯╰─╮
  ╰───┤ 🫁 ️├───╯
   ┌──│    │──┐
   │  ╰────╯  │
   │          │
🌊🌊🌊🌊🌊🌊🌊🌊🌊
---
    ┌──┬──┬──┐    
    │◌ │🫦│ ◌️│    
    └──┴╮╭┴──┘    
       ┌──┐      
  ┌────┤🌎├────┐  
  │    └──┘    │  
  │            │  
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲
---
    ╭────────╮    
    │ 💀  💀 │    
    ╰───╮╭───╯    
      ╭─╯╰─╮      
   ╭──┤    ├──╮  
   │  ╰┬──┬╯  │  
   │   ╰──╯   │  
   │          │  
   │          │   
---
    ╭──┬──┬──╮    
    │💿│ ◡│💿️│    
    ╰─╮┴╮╭┴╭─╯    
  ╰───┤─╯╰─├───╯  
  ╭───│    │───╮  
  │   ╰────╯   │  
  │            │  
 ╱│╲          ╱│╲ 
╱   ╲        ╱   ╲
---
    ╭─┴────┴─╮    
    │💿 🧠 💿️️│    
    ╰─┐    ┌─╯    
  ╰───┤ 🫦 ├───╯   
   ┌──┤    ├──┐  
   │  ╰────╯  │  
   │          │  
   │          │  
 ┌─┼─┐      ┌─┼─┐
 ---
    ╭────────╮   
    │👁 🫦 👁️│   
    ╰────────╯   
      ┌─┘└─┐     
  ┌───│ 🫁 │───┐ 
  │   └────┘   │ 
  │            │ 
 ╱│╲          ╱│╲ 
╱ │ ╲        ╱ │ ╲