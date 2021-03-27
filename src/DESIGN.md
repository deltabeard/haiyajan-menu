# Design

## Basics

- Uses SDL2 hardware accelerated drawing.
- UI elements defined using structure that may contain static elements, or dynamic elements
- The toolkit handles the placement of widgets automatically.
- The toolkit provides simple style paramters: colours, size, and alignment.
- Each UI element is placed in its own row by default.
- Each UI element is placed in its own column when embedded within a container.

## Widgets

<dl>
  <dt>Tile</dt>
  <dd>A large button that contains both an icon and a label.</dd>
  
  <dt>Button</dt>
  <dd>A button that contains a label.</dd>
  
  <dt>Toggle</dt>
  <dd>A toggle switch.</dd>
  
  <dt>Dropdown List</dt>
  <dd>A list of labels of which one can be selected.</dd>
  
  <dt>Spinner</dt>
  <dd>A spinning icon.</dd>
  
  <dt>Slider</dt>
  <dd>A handle that can be dragged accross a bar.</dd>
  
  <dt>Text Box</dt>
  <dd>Allow the user to input text.</dd>
  
</dl>

### Tile Parameters

- Size of tile, enum: small, medium, large.
- Shape of tile, enum: square, circle.
- Background Colour, SDL_Colour.
- Foreground Colour, SDL_Colour.
  - Sets the colour of the icon.
  - Sets the colour of the label if the label is located within the tile.
- Label placement, enum: inside, outside.
- Label alignment, enum: left, center, right.
- Icon, Uint32.
- Label, const char *.
- Onclick, enum: sum_menu, execute_function, set_variable.
- User, void *.

### Button Parameters

- Background Colour, SDL_Colour.
- Foreground Colour, SDL_Colour.
- Icon, Uint32.
- Label, const char *.
- Onclick, enum: sum_menu, execute_function, set_variable.
- User, void *.

### Toggle Parameters

- True Colour, SDL_Colour.
- True Text, const char *;
- False Colour, SDL_Colour.
- False Text, const char *;
- Label, const char *.
- Description, const char *.
- Onclick, enum: execute_function, set_variable.
- User, void *.

### Dropdown List Parameters

- List, const char \*\*.
- Selected, Uint32.
- Label, const char *.
- Onclick, enum: execute_function, set_variable.
- User, void *.

### Spinner Parameters

- Progress, Sint8.
  - If <0, non-standard animation.
- Label, const char *.
- Description, const char *.
- Update status, void \*(function)()
- User, void *.

### Slider Parameters

- Parameter types, enum: SINT32, FLOAT.
- Min, Sint32/float.
- Step , Sint32/float.
- Max, Sint32/float.
- Label, const char *.
- Description, const char *.
- Onchange, enum: execute_function, set_variable.
- User, void *.

### Text Box

- Text Colour, SDL_Colour.
- Keyboard, enum: Unicode, Number.
- Max, union: Uint32 (max character length), Uint32 (maximum number).
- Input, union: Uint16 \*, Uint32.

## Interface Elements

### Container

Widgets placed within this container will be placed horizontally.

### Spacer

Creates a space large enough to make the container fill the full width of the screen.
Multiple spacers can be used for alignment purposes within the container.

## UI Functions

### Message Box

### Go to element
