"""Export the stand without its redundant, undersized interior screw trace.

Run with Blender --background <source.blend> --python this_file.
Black screws remain in the visual LODs. Surrounding knobs and upright already
provide their exterior trace surfaces.
"""
from pathlib import Path
import bpy

root = Path(__file__).resolve().parents[2]
out = root / 'Assets' / 'HPC_Stand' / 'HPC_Stand.fbx'
bpy.ops.object.select_all(action='DESELECT')
objects = list(bpy.data.collections['HPC_GAME_LODS'].objects)
objects += [o for o in bpy.data.collections['HPC_COLLISION'].objects
            if o.name != 'UTM_HPC_FireView_screw']
for obj in objects:
    obj.hide_set(False)
    obj.select_set(True)
bpy.context.view_layer.objects.active = bpy.data.objects['HPC_Stand_LOD0']
bpy.ops.export_scene.fbx(filepath=str(out), use_selection=True,
    object_types={'MESH'}, use_custom_props=True, global_scale=1,
    apply_unit_scale=True, use_mesh_modifiers=True, add_leaf_bones=False,
    bake_anim=False, path_mode='AUTO', mesh_smooth_type='FACE')
print('HPC_EXPORT_OK', len(objects), out)
