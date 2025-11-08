import bpy

obdata = bpy.context.object.data

f=open("/tmp/object.h","w")

vtx=0
edge=0

print('static const long cubeX[] = {\n    ', end='', file=f)
i=0
for v in obdata.vertices:
    print('TO_FIX({:.5}), '.format(v.co.x), end='', file=f)
    i=i+1
    vtx=vtx+1
    if i % 3 == 0: print('\n    ',end='',file=f)
print('};', file=f)

i=0
print('static const long cubeY[] = {\n    ', end='', file=f)
for v in obdata.vertices:
    print('TO_FIX({:.5}), '.format(v.co.y), end='', file=f)
    i=i+1
    if i % 3 == 0: print('\n    ',end='',file=f)
print('};', file=f)

i=0
print('static const long cubeZ[] = {\n    ', end='', file=f)
for v in obdata.vertices:
    print('TO_FIX({:.5}), '.format(v.co.z), end='', file=f)
    i=i+1
    if i % 3 == 0: print('\n    ',end='',file=f)
print('};', file=f)

i=0
print('static const int edges[] = {', file=f)
for e in obdata.edges:
    print('{}, {}, '.format(e.vertices[0], e.vertices[1]), end='', file=f)
    i=i+1
    edge=edge+2
    if i % 3 == 0: print('\n    ',end='',file=f)
print('};', file=f)

print('#define NUM_VTX {}'.format(vtx), file=f)
print('#define NUM_EDGE {}'.format(edge), file=f)

f.close()
