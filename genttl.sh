#!/bin/bash

IDX=0

cat > lv2ttl/mixtri.lv2.ttl.in << EOF


@LV2NAME@:@INSTANCE@@URI_SUFFIX@
  a lv2:Plugin, doap:Project;
  doap:license <http://usefulinc.com/doap/licenses/gpl> ;
  doap:maintainer <http://gareus.org/rgareus#me> ;
  doap:name "Mixer'n'Trigger@NAME_SUFFIX@";
  lv2:optionalFeature lv2:hardRTCapable ;
  ui:ui @LV2NAME@:@UI@ ;
  lv2:port
  [
EOF

i=1; while test $i -lt 5; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:AudioPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "in${i}" ;
    lv2:name "Audio Input ${i}" ;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done

i=1; while test $i -lt 4; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:AudioPort ,
      lv2:OutputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "out${i}" ;
    lv2:name "Audio Output ${i}" ;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done

cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:AudioPort ,
      lv2:OutputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "trigger_out" ;
    lv2:name "Audio Output Trigger" ;
  ] , [
EOF
IDX=$[$IDX + 1]

i=0; while test $i -lt 4; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "gain_in_${i}" ;
    lv2:name "Input Gain ${i}" ;
    lv2:minimum  -60.0;
    lv2:maximum  30.0;
    lv2:default  0.0;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done

i=0; while test $i -lt 4; do
	j=0; while test $j -lt 3; do
	AMP=0.0
	if test $j -eq $i; then AMP=1.0; fi
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "mix_${i}_${j}" ;
    lv2:name "Mix ${i} ${j}" ;
    lv2:minimum -6.0;
    lv2:maximum  6.0;
    lv2:default  ${AMP};
    lv2:scalePoint [ rdfs:label "invert"; rdf:value -1.0 ; ] ;
    lv2:scalePoint [ rdfs:label "off"; rdf:value 0.0 ; ] ;
    lv2:scalePoint [ rdfs:label "on"; rdf:value 1.0 ; ] ;
  ] , [
EOF
		j=$[$j + 1]
		IDX=$[$IDX + 1]
	done
	i=$[$i + 1]
done

i=0; while test $i -lt 4; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "delay_in_${i}" ;
    lv2:name "Delay input ${i}" ;
    lv2:minimum  0.0;
    lv2:maximum  48000.0;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done

i=0; while test $i -lt 3; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "delay_out_${i}" ;
    lv2:name "Delay output ${i}" ;
    lv2:minimum  0.0;
    lv2:maximum  48000.0;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done

i=0; while test $i -lt 4; do
cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "mod_in_${i}" ;
    lv2:name "Input ${i} Mode" ;
    lv2:minimum  0.0;
    lv2:maximum  3.0;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
    lv2:portProperty lv2:enumeration ;
    lv2:scalePoint [ rdfs:label "none"; rdf:value 0.0 ; ] ;
    lv2:scalePoint [ rdfs:label "mute"; rdf:value 1.0 ; ] ;
    lv2:scalePoint [ rdfs:label "high-pass filter"; rdf:value 2.0 ; ] ;
  ] , [
EOF
	i=$[$i + 1]
	IDX=$[$IDX + 1]
done



cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index ${IDX} ;
    lv2:symbol "trigger_channel" ;
    lv2:name "Trigger Channel" ;
    lv2:minimum  0.0;
    lv2:maximum  3.0;
    lv2:default  4.0;
    lv2:portProperty lv2:integer ;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 1));
    lv2:symbol "trigger_mode" ;
    lv2:name "Trigger Mode" ;
    lv2:minimum  0.0;
    lv2:maximum  2.0;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
    lv2:portProperty lv2:enumeration ;
    lv2:scalePoint [ rdfs:label "passtru"; rdf:value 0.0 ; ] ;
    lv2:scalePoint [ rdfs:label "LTC"; rdf:value 1.0 ; ] ;
    lv2:scalePoint [ rdfs:label "Edge"; rdf:value 2.0 ; ] ;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 2));
    lv2:symbol "trigger_edge" ;
    lv2:name "Trigger Edge Mode" ;
    lv2:minimum  0.0;
    lv2:maximum  3.0;
    lv2:default  1.0;
    lv2:portProperty lv2:integer ;
    lv2:portProperty lv2:enumeration ;
    lv2:scalePoint [ rdfs:label "off"; rdf:value 0.0 ; ] ;
    lv2:scalePoint [ rdfs:label "rising-edge"; rdf:value 1.0 ; ] ;
    lv2:scalePoint [ rdfs:label "falling-edge"; rdf:value 2.0 ; ] ;
    lv2:scalePoint [ rdfs:label "any-edge"; rdf:value 3.0 ; ] ;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 3));
    lv2:symbol "trigger_level1" ;
    lv2:name "Trigger Level1" ;
    lv2:minimum  -1.0;
    lv2:maximum  1.0;
    lv2:default  0.0;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 4));
    lv2:symbol "trigger_level2" ;
    lv2:name "Trigger Level2" ;
    lv2:minimum  -1.0;
    lv2:maximum  1.0;
    lv2:default  0.0;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 5));
    lv2:symbol "trigger_time1" ;
    lv2:name "Trigger time1" ;
    lv2:minimum  0.0;
    lv2:maximum  192000;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
  ] , [
    a lv2:ControlPort ,
      lv2:InputPort ;
    lv2:index $(($IDX + 6));
    lv2:symbol "trigger_time2" ;
    lv2:name "Trigger time2" ;
    lv2:minimum  0.0;
    lv2:maximum  192000;
    lv2:default  0.0;
    lv2:portProperty lv2:integer ;
EOF

cat >> lv2ttl/mixtri.lv2.ttl.in << EOF
  ] ;
  rdfs:comment "Matrix mixer with trigger intended for sisco.lv2." ;
  .
EOF
