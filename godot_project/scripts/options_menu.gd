extends Control


@onready var music_volume_label: RichTextLabel = $MusicVolumeContainer/MusicVolumeSliderLabel
@onready var music_volume_slider: HSlider = $MusicVolumeContainer/MusicVolumeSlider
@onready var vfx_volume_label: RichTextLabel = $VFXVolumeContainer/VFXVolumeSliderLabel
@onready var vfx_volume_slider: HSlider = $VFXVolumeContainer/VFXVolumeSlider
@onready var back_button: Button = $BackButton

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	back_button.pressed.connect(GameManager.go_to_start.bind())


func _on_music_volume_slider_value_changed(value: float) -> void:
	# TODO update GameManager value 
	
	music_volume_label.text = str(int(value * 100)) + "%"


func _on_vfx_volume_slider_value_changed(value: float) -> void:
	# TODO update GameManager value
	
	vfx_volume_label.text = str(int(value * 100)) + "%"
