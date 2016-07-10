inputqqqq=workspace/vv/VVJJConfig/decorated/2000_HVT_WZ_decorated.root
inputvvqq=workspace/vv/vvqqConfig/decorated/2000_HVT_decorated.root
inputllqq=workspace/vv/llqqConfig/decorated/2000_HVT_decorated.root
inputlvqq=workspace/vv/lvqqConfig/decorated/2000_HVT_decorated.root
inputvh=workspace/vv/VHConfig/decorated/2000_HVT_decorated.root

# template=cfg/vv/combined_VV_template.xml

# cat ${template} | sed "s|#INPUTqqqq#|${inputqqqq}|g" | sed "s|#INPUTvvqq#|${inputvvqq}|g" | sed "s|#INPUTllqq#|${inputllqq}|g" | sed "s|#INPUTlvqq#|${inputlvqq}|g" | sed "s|#INPUTvh#|${inputvh}|g" > cfg/vv/combined_VV_obs.xml
# python python/createXML.py cfg/vv/combined_VV_obs.xml

# manager -w combine -x cfg/vv/combined_VV_obs.xml -s true -t 2 -f workspace/vv/CombinationConfig/VV_obs.root >& workspace/vv/CombinationConfig/VV_obs.log &

template=cfg/vv/combined_VVVH_template.xml
cat ${template} | sed "s|#INPUTqqqq#|${inputqqqq}|g" | sed "s|#INPUTvvqq#|${inputvvqq}|g" | sed "s|#INPUTllqq#|${inputllqq}|g" | sed "s|#INPUTlvqq#|${inputlvqq}|g" | sed "s|#INPUTvh#|${inputvh}|g" > cfg/vv/combined_VVVH_obs.xml
python python/createXML.py cfg/vv/combined_VVVH_obs.xml
manager -w combine -x cfg/vv/combined_VVVH_obs.xml -s true -t 2 -f workspace/vv/CombinationConfig/VVVH_obs.root >& workspace/vv/CombinationConfig/VVVH_obs.log &
