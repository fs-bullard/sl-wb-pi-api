#!/usr/bin/env python3
"""
Simple test client for Western Blot Capture API

Tests basic functionality and demonstrates API usage.
"""

import sys
import argparse
import requests
import numpy as np
from pathlib import Path
from PIL import Image


def test_init(base_url):
    """Test init endpoint."""
    print("Testing /init endpoint...")
    try:
        response = requests.post(f"{base_url}/init", timeout=10)
        if response.status_code == 200:
            data = response.json()
            print(f"  ✓ Device initialized: {data}")
            return True
        else:
            print(f"  ✗ Initialization failed: {response.status_code}")
            try:
                error = response.json()
                print(f"    Error: {error.get('message', 'Unknown error')}")
            except:
                print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"  ✗ Initialization error: {e}")
        return False


def test_health(base_url):
    """Test health endpoint."""
    print("Testing /health endpoint...")
    try:
        response = requests.get(f"{base_url}/health", timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"  ✓ Health check passed: {data}")
            return True
        elif response.status_code == 503:
            data = response.json()
            print(f"  ⚠ Device not ready: {data}")
            return False
        else:
            print(f"  ✗ Health check failed: {response.status_code}")
            print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"  ✗ Health check error: {e}")
        return False


def test_led_on(base_url):
    """Test LED on endpoint."""
    print("Testing /led_on endpoint...")
    try:
        response = requests.post(f"{base_url}/led_on", timeout=5)
        if response.status_code == 200:
            print(f"  ✓ LED turned on")
            return True
        else:
            print(f"  ✗ LED on failed: {response.status_code}")
            try:
                error = response.json()
                print(f"    Error: {error.get('message', 'Unknown error')}")
            except:
                print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"  ✗ LED on error: {e}")
        return False


def test_led_off(base_url):
    """Test LED off endpoint."""
    print("Testing /led_off endpoint...")
    try:
        response = requests.post(f"{base_url}/led_off", timeout=5)
        if response.status_code == 200:
            print(f"  ✓ LED turned off")
            return True
        else:
            print(f"  ✗ LED off failed: {response.status_code}")
            try:
                error = response.json()
                print(f"    Error: {error.get('message', 'Unknown error')}")
            except:
                print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"  ✗ LED off error: {e}")
        return False


def test_capture(base_url, exposure_ms, led_val, output_file=None):
    """Test capture endpoint."""
    print(f"Testing /capture endpoint (exposure: {exposure_ms}ms, led_val: {led_val})...")
    try:
        response = requests.post(
            f"{base_url}/capture",
            json={"exposure_ms": exposure_ms, "led": led_val},
            timeout=exposure_ms/1000 + 30  # exposure + 30s buffer
        )

        if response.status_code == 200:
            # Get metadata from headers
            width = int(response.headers.get('X-Frame-Width', 0))
            height = int(response.headers.get('X-Frame-Height', 0))
            exposure = response.headers.get('X-Exposure-Ms')

            print(f"  ✓ Capture successful:")
            print(f"    Resolution: {width}x{height}")
            print(f"    Exposure: {exposure}ms")
            print(f"    Led value: {led_val}")
            print(f"    Data size: {len(response.content)} bytes")

            # Verify data size (assuming uint16 pixels)
            expected_size = width * height * 2
            if len(response.content) == expected_size:
                print(f"    Data size matches expected: {expected_size} bytes")
            else:
                print(f"    WARNING: Data size mismatch! Expected {expected_size}, got {len(response.content)}")

            # Save to file if requested
            if output_file:
                output_path = Path(output_file)
                output_path.write_bytes(response.content)
                print(f"    Saved to: {output_file}")

                # Show basic statistics if numpy available
                try:
                    image = np.frombuffer(response.content, dtype=np.uint16)
                    image = image.reshape((height, width))

                    Image.fromarray(image).save('test.tiff')
                    print(f"    Statistics:")
                    print(f"      Min: {image.min()}")
                    print(f"      Max: {image.max()}")
                    print(f"      Mean: {image.mean():.1f}")
                    print(f"      Std: {image.std():.1f}")
                except ImportError:
                    print(f"    (Install numpy for image statistics)")

            return True
        else:
            print(f"  ✗ Capture failed: {response.status_code}")
            try:
                error = response.json()
                print(f"    Error: {error.get('message', 'Unknown error')}")
            except:
                print(f"    {response.text}")
            return False

    except requests.exceptions.Timeout:
        print(f"  ✗ Capture timeout (waited {exposure_ms/1000 + 30}s)")
        return False
    except Exception as e:
        print(f"  ✗ Capture error: {e}")
        return False


def test_shutdown(base_url):
    """Test shutdown endpoint."""
    print("Testing /shutdown endpoint...")
    try:
        response = requests.post(f"{base_url}/shutdown", timeout=5)
        if response.status_code == 200:
            data = response.json()
            print(f"  ✓ Device shutdown: {data}")
            return True
        else:
            print(f"  ✗ Shutdown failed: {response.status_code}")
            try:
                error = response.json()
                print(f"    Error: {error.get('message', 'Unknown error')}")
            except:
                print(f"    {response.text}")
            return False
    except Exception as e:
        print(f"  ✗ Shutdown error: {e}")
        return False


def test_invalid_exposure(base_url):
    """Test error handling with invalid exposure times."""
    print("Testing error handling...")

    # Test too low
    print("  Testing exposure too low (5ms)...")
    try:
        response = requests.post(
            f"{base_url}/capture",
            json={"exposure_ms": 5},
            timeout=5
        )
        if response.status_code == 400:
            print(f"    ✓ Correctly rejected with 400")
        else:
            print(f"    ✗ Expected 400, got {response.status_code}")
    except Exception as e:
        print(f"    ✗ Error: {e}")

    # Test too high
    print("  Testing exposure too high (20000ms)...")
    try:
        response = requests.post(
            f"{base_url}/capture",
            json={"exposure_ms": 20000},
            timeout=5
        )
        if response.status_code == 400:
            print(f"    ✓ Correctly rejected with 400")
        else:
            print(f"    ✗ Expected 400, got {response.status_code}")
    except Exception as e:
        print(f"    ✗ Error: {e}")

    # Test missing parameter
    print("  Testing missing exposure_ms...")
    try:
        response = requests.post(
            f"{base_url}/capture",
            json={},
            timeout=5
        )
        if response.status_code == 400:
            print(f"    ✓ Correctly rejected with 400")
        else:
            print(f"    ✗ Expected 400, got {response.status_code}")
    except Exception as e:
        print(f"    ✗ Error: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Test client for Western Blot Capture API",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run full test sequence
  python test_client.py --host 192.168.1.100 --full-test

  # Test specific endpoint
  python test_client.py --host 192.168.1.100 --test init
  python test_client.py --host 192.168.1.100 --test capture --exposure 1000

  # Quick capture test
  python test_client.py --host 192.168.1.100 --exposure 500 --output frame.raw
        """
    )
    parser.add_argument(
        '--host',
        default='localhost',
        help='Pi hostname or IP address (default: localhost)'
    )
    parser.add_argument(
        '--port',
        type=int,
        default=5000,
        help='API port (default: 5000)'
    )
    parser.add_argument(
        '--exposure',
        type=int,
        default=100,
        help='Exposure time in ms (default: 100)'
    )
    parser.add_argument(
        '--led',
        type=float,
        default=0.01,
        help='LED value (default: 0.01)'
    )
    parser.add_argument(
        '--output',
        help='Save captured image to file (e.g., capture.raw)'
    )
    parser.add_argument(
        '--test',
        choices=['health', 'init', 'led_on', 'led_off', 'capture', 'shutdown'],
        help='Run a specific test only'
    )
    parser.add_argument(
        '--full-test',
        action='store_true',
        help='Run full test sequence (init, health, LED, capture, shutdown)'
    )
    parser.add_argument(
        '--test-errors',
        action='store_true',
        help='Run error handling tests'
    )

    args = parser.parse_args()

    base_url = f"http://{args.host}:{args.port}"

    print("=" * 60)
    print("Western Blot Capture API Test Client")
    print("=" * 60)
    print(f"Base URL: {base_url}")
    print()

    # Run specific test if requested
    if args.test:
        if args.test == 'health':
            success = test_health(base_url)
        elif args.test == 'init':
            success = test_init(base_url)
        elif args.test == 'led_on':
            success = test_led_on(base_url)
        elif args.test == 'led_off':
            success = test_led_off(base_url)
        elif args.test == 'capture':
            success = test_capture(base_url, args.exposure, args.led, args.output)
        elif args.test == 'shutdown':
            success = test_shutdown(base_url)

        print()
        print("=" * 60)
        if success:
            print(f"Test '{args.test}' passed!")
        else:
            print(f"Test '{args.test}' failed.")
            sys.exit(1)
        return

    # Run full test sequence if requested
    if args.full_test:
        print("[1/6] Initializing device...")
        if not test_init(base_url):
            print("\n✗ Device initialization failed. Cannot continue.")
            sys.exit(1)
        print()

        print("[2/6] Checking health...")
        test_health(base_url)
        print()

        print("[3/6] Testing LED on...")
        test_led_on(base_url)
        print()

        print("[4/6] Capturing frame...")
        test_capture(base_url, args.exposure, args.led, args.output or "test_frame.raw")
        print()

        print("[5/6] Testing LED off...")
        test_led_off(base_url)
        print()

        print("[6/6] Shutting down...")
        test_shutdown(base_url)
        print()

        if args.test_errors:
            print("[Extra] Testing error handling...")
            test_invalid_exposure(base_url)
            print()

        print("=" * 60)
        print("✓ Full test sequence completed!")
        print("=" * 60)
        return

    # Default: just test health and capture
    if not test_health(base_url):
        print("\n⚠ Health check shows device not ready.")
        print("  Run with --full-test to initialize the device first,")
        print("  or use --test init to initialize manually.")
        sys.exit(1)

    print()

    # Test capture
    success = test_capture(base_url, args.exposure, args.led, args.output)

    if args.test_errors:
        print()
        test_invalid_exposure(base_url)

    print()
    print("=" * 60)
    if success:
        print("Tests completed successfully!")
    else:
        print("Some tests failed.")
        sys.exit(1)


if __name__ == '__main__':
    main()
