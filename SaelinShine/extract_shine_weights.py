"""
Extract weights from shine.pt checkpoint into C++ header.
Outputs ShineModelWeights.h with constexpr arrays.
"""

import torch
import numpy as np
import os

def format_array(name, arr, indent="    "):
    """Format a numpy array as a C++ constexpr float array."""
    flat = arr.flatten()
    lines = [f"{indent}static constexpr float {name}[{len(flat)}] = {{"]

    for i in range(0, len(flat), 8):
        chunk = flat[i:i+8]
        vals = ", ".join(f"{v:.8f}f" for v in chunk)
        if i + 8 < len(flat):
            vals += ","
        lines.append(f"{indent}    {vals}")

    lines.append(f"{indent}}};")
    return "\n".join(lines)

def main():
    model_path = os.path.join(os.path.dirname(__file__), '..', 'saelin_shine', 'shine.pt')

    if not os.path.exists(model_path):
        print(f"Error: {model_path} not found")
        return

    checkpoint = torch.load(model_path, map_location='cpu', weights_only=False)

    state = checkpoint['model_state_dict']
    X_mean = np.array(checkpoint['X_mean'])
    X_std = np.array(checkpoint['X_std'])

    print(f"Model: shine.pt")
    print(f"  Input size: {checkpoint['input_size']}")
    print(f"  Hidden size: {checkpoint['hidden_size']}")
    print(f"  Output size: {checkpoint['output_size']}")
    print(f"  Val loss: {checkpoint['val_loss']:.6f}")
    print(f"  Targets: {checkpoint['target_names']}")
    print()

    for key, tensor in state.items():
        arr = tensor.numpy()
        print(f"  {key}: {arr.shape}")

    # Build header
    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("// Auto-generated from shine.pt - DO NOT EDIT")
    lines.append("// Network: 30 -> 64 -> 64 -> 32 -> 2 (presence, air)")
    lines.append("// Architecture: Linear+LeakyReLU x3, then Linear output")
    lines.append("")
    lines.append("namespace shine {")
    lines.append("namespace weights {")
    lines.append("")

    # Normalization stats
    lines.append(format_array("X_mean", X_mean))
    lines.append("")
    lines.append(format_array("X_std", X_std))
    lines.append("")

    # Layer weights and biases
    # network.0 = layer1 (30->64)
    lines.append(format_array("layer1_weight", state['network.0.weight'].numpy()))
    lines.append("")
    lines.append(format_array("layer1_bias", state['network.0.bias'].numpy()))
    lines.append("")

    # network.3 = layer2 (64->64)
    lines.append(format_array("layer2_weight", state['network.3.weight'].numpy()))
    lines.append("")
    lines.append(format_array("layer2_bias", state['network.3.bias'].numpy()))
    lines.append("")

    # network.6 = layer3 (64->32)
    lines.append(format_array("layer3_weight", state['network.6.weight'].numpy()))
    lines.append("")
    lines.append(format_array("layer3_bias", state['network.6.bias'].numpy()))
    lines.append("")

    # network.8 = output (32->2)
    lines.append(format_array("output_weight", state['network.8.weight'].numpy()))
    lines.append("")
    lines.append(format_array("output_bias", state['network.8.bias'].numpy()))
    lines.append("")

    lines.append("} // namespace weights")
    lines.append("} // namespace shine")
    lines.append("")

    output_path = os.path.join(os.path.dirname(__file__), 'Source', 'ML', 'ShineModelWeights.h')
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, 'w') as f:
        f.write("\n".join(lines))

    total_params = sum(t.numel() for t in state.values()) + len(X_mean) + len(X_std)
    print(f"\nWrote {output_path}")
    print(f"Total parameters: {total_params}")

if __name__ == "__main__":
    main()
