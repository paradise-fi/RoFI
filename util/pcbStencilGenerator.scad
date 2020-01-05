module stencilFrame(outlineDxf, height, width, clearance) {
    linear_extrude(height = height)
        difference() {
            offset(r = width + clearance) import(file = outlineDxf);
            offset(r = clearance) import(file = outlineDxf);
        };
};

module stencilSubstrate(outlineDxf, thickness, frameHeight,
    frameWidth, frameClearance)
{
    difference() {
        linear_extrude(height = thickness + frameHeight)
            offset(r = frameWidth + frameClearance)
            import(file = outlineDxf);
        translate([0, 0, thickness])
            linear_extrude(height = thickness + frameHeight)
            offset(r = frameClearance)
            import(file = outlineDxf);
    }
}

module stencil(outlineDxf, holesDxf, thickness = 0.2, frameHeight = 1,
    frameWidth = 1, frameClearance = 0.1, front = true)
{
    zScale = front ? -1 : 1;
    scale([1, 1, zScale])
        difference() {
            scale([1, 1, zScale]) stencilSubstrate(outlineDxf, thickness,
                frameHeight, frameWidth, frameClearance);
            linear_extrude(height = 4 * thickness, center = true)
                import(file = holesDxf);
        };
}

$fa = 0.4;
$fs = 0.4;
thickness = 0.2;
frameHeight = 1;
frameWidth = 1;
frameClearance = 0.1;
stencil(outline, mask, thickness = thickness, frameHeight = frameHeight,
    frameWidth = frameWidth, frameClearance = frameClearance, front=front);